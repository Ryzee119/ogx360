/* Copyright (C) 2012 Kristian Lauszus, TKJ Electronics. All rights reserved.

This software may be distributed and modified under the terms of the GNU
General Public License version 2 (GPL2) as published by the Free Software
Foundation and appearing in the file GPL2.TXT included in the packaging of
this file. Please note that GPL2 Section 2[b] requires that all works based
on this software must also be made publicly available under the terms of
the GPL2 ("Copyleft").

Contact information
-------------------

Kristian Lauszus, TKJ Electronics
Web      :  http://www.tkjelectronics.com
e-mail   :  kristianl@tkjelectronics.com

getBatteryLevel and checkStatus functions made by timstamp.co.uk found using BusHound from Perisoft.net
*/

#include "XBOXRECV.h"
// To enable serial debugging see "settings.h"
//#define EXTRADEBUG // Uncomment to get even more debugging data
//#define PRINTREPORT // Uncomment to print the report send by the Xbox 360 Controller

XBOXRECV::XBOXRECV(USB *p) :
pUsb(p), // pointer to USB class instance - mandatory
bAddress(0), // device address - mandatory
bPollEnable(false) { // don't start polling before dongle is connected
    for(uint8_t i = 0; i < XBOX_MAX_ENDPOINTS; i++) {
        epInfo[i].epAddr = 0;
        epInfo[i].maxPktSize = (i) ? 0 : 8;
        epInfo[i].bmSndToggle = 0;
        epInfo[i].bmRcvToggle = 0;
        epInfo[i].bmNakPower = (i) ? USB_NAK_NOWAIT : USB_NAK_MAX_POWER;
    }

    if(pUsb) // register in USB subsystem
        pUsb->RegisterDeviceClass(this); //set devConfig[] entry
}

uint8_t XBOXRECV::ConfigureDevice(uint8_t parent, uint8_t port, bool lowspeed) {
    const uint8_t constBufSize = sizeof (USB_DEVICE_DESCRIPTOR);
    uint8_t buf[constBufSize];
    USB_DEVICE_DESCRIPTOR * udd = reinterpret_cast<USB_DEVICE_DESCRIPTOR*>(buf);
    uint8_t rcode;
    UsbDevice *p = NULL;
    EpInfo *oldep_ptr = NULL;
    uint16_t PID, VID;

    AddressPool &addrPool = pUsb->GetAddressPool(); // Get memory address of USB device address pool

    if(bAddress) { // Check if address has already been assigned to an instance
        return USB_ERROR_CLASS_INSTANCE_ALREADY_IN_USE;
    }

    p = addrPool.GetUsbDevicePtr(0); // Get pointer to pseudo device with address 0 assigned

    if(!p) {
        return USB_ERROR_ADDRESS_NOT_FOUND_IN_POOL;
    }

    if(!p->epinfo) {
        return USB_ERROR_EPINFO_IS_NULL;
    }

    oldep_ptr = p->epinfo; // Save old pointer to EP_RECORD of address 0
    p->epinfo = epInfo; // Temporary assign new pointer to epInfo to p->epinfo in order to avoid toggle inconsistence
    p->lowspeed = lowspeed;

    rcode = pUsb->getDevDescr(0, 0, constBufSize, (uint8_t*)buf); // Get device descriptor - addr, ep, nbytes, data

    p->epinfo = oldep_ptr; // Restore p->epinfo

    if(rcode)
        goto FailGetDevDescr;

    VID = udd->idVendor;
    PID = udd->idProduct;

    if(!VIDPIDOK(VID,PID))
        goto FailUnknownDevice;

    bAddress = addrPool.AllocAddress(parent, false, port); // Allocate new address according to device class

    if(!bAddress) {
        return USB_ERROR_OUT_OF_ADDRESS_SPACE_IN_POOL;
    }

    epInfo[0].maxPktSize = udd->bMaxPacketSize0; // Extract Max Packet Size from device descriptor

    delay(20); // Wait a little before resetting device

    return USB_ERROR_CONFIG_REQUIRES_ADDITIONAL_RESET;

    /* Diagnostic messages */
    FailGetDevDescr:
    #ifdef DEBUG_USB_HOST
    NotifyFailGetDevDescr(rcode);
    #endif
    if(rcode != hrJERR)
        rcode = USB_ERROR_FailGetDevDescr;
    goto Fail;

    FailUnknownDevice:
    #ifdef DEBUG_USB_HOST
    NotifyFailUnknownDevice(VID, PID);
    #endif
    rcode = USB_DEV_CONFIG_ERROR_DEVICE_NOT_SUPPORTED;

    Fail:
    #ifdef DEBUG_USB_HOST
    Notify(PSTR("\r\nXbox 360 Init Failed, error code: "), 0x80);
    NotifyFail(rcode);
    #endif
    Release();
    return rcode;
};

uint8_t XBOXRECV::Init(uint8_t parent __attribute__((unused)), uint8_t port __attribute__((unused)), bool lowspeed) {
    uint8_t rcode;

    AddressPool &addrPool = pUsb->GetAddressPool();
    #ifdef EXTRADEBUG
    Notify(PSTR("\r\nBTD Init"), 0x80);
    #endif
    UsbDevice *p = addrPool.GetUsbDevicePtr(bAddress); // Get pointer to assigned address record

    if(!p) {
        #ifdef DEBUG_USB_HOST
        Notify(PSTR("\r\nAddress not found"), 0x80);
        #endif
        return USB_ERROR_ADDRESS_NOT_FOUND_IN_POOL;
    }

    delay(300); // Assign new address to the device

    rcode = pUsb->setAddr(0, 0, bAddress); // Assign new address to the device
    if(rcode) {
        #ifdef DEBUG_USB_HOST
        Notify(PSTR("\r\nsetAddr: "), 0x80);
        D_PrintHex<uint8_t > (rcode, 0x80);
        #endif
        p->lowspeed = false;
        goto Fail;
    }
    #ifdef EXTRADEBUG
    Notify(PSTR("\r\nAddr: "), 0x80);
    D_PrintHex<uint8_t > (bAddress, 0x80);
    #endif

    p->lowspeed = false;

    p = addrPool.GetUsbDevicePtr(bAddress); // Get pointer to assigned address record
    if(!p) {
        #ifdef DEBUG_USB_HOST
        Notify(PSTR("\r\nAddress not found"), 0x80);
        #endif
        return USB_ERROR_ADDRESS_NOT_FOUND_IN_POOL;
    }

    p->lowspeed = lowspeed;

    rcode = pUsb->setEpInfoEntry(bAddress, 1, epInfo); // Assign epInfo to epinfo pointer - only EP0 is known
    if(rcode)
    goto FailSetDevTblEntry;

    /* The application will work in reduced host mode, so we can save program and data
    memory space. After verifying the VID we will use known values for the
    configuration values for device, interface, endpoints and HID for the XBOX360 Wireless receiver */

    /* Initialize data structures for endpoints of device */
    epInfo[ XBOX_INPUT_PIPE_1 ].epAddr = 0x01; // XBOX 360 report endpoint - poll interval 1ms
    epInfo[ XBOX_INPUT_PIPE_1 ].epAttribs = USB_TRANSFER_TYPE_INTERRUPT;
    epInfo[ XBOX_INPUT_PIPE_1 ].bmNakPower = USB_NAK_NOWAIT; // Only poll once for interrupt endpoints
    epInfo[ XBOX_INPUT_PIPE_1 ].maxPktSize = EP_MAXPKTSIZE;
    epInfo[ XBOX_INPUT_PIPE_1 ].bmSndToggle = 0;
    epInfo[ XBOX_INPUT_PIPE_1 ].bmRcvToggle = 0;
    epInfo[ XBOX_OUTPUT_PIPE_1 ].epAddr = 0x01; // XBOX 360 output endpoint - poll interval 8ms
    epInfo[ XBOX_OUTPUT_PIPE_1 ].epAttribs = USB_TRANSFER_TYPE_INTERRUPT;
    epInfo[ XBOX_OUTPUT_PIPE_1 ].bmNakPower = USB_NAK_NOWAIT; // Only poll once for interrupt endpoints
    epInfo[ XBOX_OUTPUT_PIPE_1 ].maxPktSize = EP_MAXPKTSIZE;
    epInfo[ XBOX_OUTPUT_PIPE_1 ].bmSndToggle = 0;
    epInfo[ XBOX_OUTPUT_PIPE_1 ].bmRcvToggle = 0;

    epInfo[ XBOX_INPUT_PIPE_2 ].epAddr = 0x03; // XBOX 360 report endpoint - poll interval 1ms
    epInfo[ XBOX_INPUT_PIPE_2 ].epAttribs = USB_TRANSFER_TYPE_INTERRUPT;
    epInfo[ XBOX_INPUT_PIPE_2 ].bmNakPower = USB_NAK_NOWAIT; // Only poll once for interrupt endpoints
    epInfo[ XBOX_INPUT_PIPE_2 ].maxPktSize = EP_MAXPKTSIZE;
    epInfo[ XBOX_INPUT_PIPE_2 ].bmSndToggle = 0;
    epInfo[ XBOX_INPUT_PIPE_2 ].bmRcvToggle = 0;
    epInfo[ XBOX_OUTPUT_PIPE_2 ].epAddr = 0x03; // XBOX 360 output endpoint - poll interval 8ms
    epInfo[ XBOX_OUTPUT_PIPE_2 ].epAttribs = USB_TRANSFER_TYPE_INTERRUPT;
    epInfo[ XBOX_OUTPUT_PIPE_2 ].bmNakPower = USB_NAK_NOWAIT; // Only poll once for interrupt endpoints
    epInfo[ XBOX_OUTPUT_PIPE_2 ].maxPktSize = EP_MAXPKTSIZE;
    epInfo[ XBOX_OUTPUT_PIPE_2 ].bmSndToggle = 0;
    epInfo[ XBOX_OUTPUT_PIPE_2 ].bmRcvToggle = 0;

    epInfo[ XBOX_INPUT_PIPE_3 ].epAddr = 0x05; // XBOX 360 report endpoint - poll interval 1ms
    epInfo[ XBOX_INPUT_PIPE_3 ].epAttribs = USB_TRANSFER_TYPE_INTERRUPT;
    epInfo[ XBOX_INPUT_PIPE_3 ].bmNakPower = USB_NAK_NOWAIT; // Only poll once for interrupt endpoints
    epInfo[ XBOX_INPUT_PIPE_3 ].maxPktSize = EP_MAXPKTSIZE;
    epInfo[ XBOX_INPUT_PIPE_3 ].bmSndToggle = 0;
    epInfo[ XBOX_INPUT_PIPE_3 ].bmRcvToggle = 0;
    epInfo[ XBOX_OUTPUT_PIPE_3 ].epAddr = 0x05; // XBOX 360 output endpoint - poll interval 8ms
    epInfo[ XBOX_OUTPUT_PIPE_3 ].epAttribs = USB_TRANSFER_TYPE_INTERRUPT;
    epInfo[ XBOX_OUTPUT_PIPE_3 ].bmNakPower = USB_NAK_NOWAIT; // Only poll once for interrupt endpoints
    epInfo[ XBOX_OUTPUT_PIPE_3 ].maxPktSize = EP_MAXPKTSIZE;
    epInfo[ XBOX_OUTPUT_PIPE_3 ].bmSndToggle = 0;
    epInfo[ XBOX_OUTPUT_PIPE_3 ].bmRcvToggle = 0;

    epInfo[ XBOX_INPUT_PIPE_4 ].epAddr = 0x07; // XBOX 360 report endpoint - poll interval 1ms
    epInfo[ XBOX_INPUT_PIPE_4 ].epAttribs = USB_TRANSFER_TYPE_INTERRUPT;
    epInfo[ XBOX_INPUT_PIPE_4 ].bmNakPower = USB_NAK_NOWAIT; // Only poll once for interrupt endpoints
    epInfo[ XBOX_INPUT_PIPE_4 ].maxPktSize = EP_MAXPKTSIZE;
    epInfo[ XBOX_INPUT_PIPE_4 ].bmSndToggle = 0;
    epInfo[ XBOX_INPUT_PIPE_4 ].bmRcvToggle = 0;
    epInfo[ XBOX_OUTPUT_PIPE_4 ].epAddr = 0x07; // XBOX 360 output endpoint - poll interval 8ms
    epInfo[ XBOX_OUTPUT_PIPE_4 ].epAttribs = USB_TRANSFER_TYPE_INTERRUPT;
    epInfo[ XBOX_OUTPUT_PIPE_4 ].bmNakPower = USB_NAK_NOWAIT; // Only poll once for interrupt endpoints
    epInfo[ XBOX_OUTPUT_PIPE_4 ].maxPktSize = EP_MAXPKTSIZE;
    epInfo[ XBOX_OUTPUT_PIPE_4 ].bmSndToggle = 0;
    epInfo[ XBOX_OUTPUT_PIPE_4 ].bmRcvToggle = 0;

    rcode = pUsb->setEpInfoEntry(bAddress, 9, epInfo);
    if(rcode)
    goto FailSetDevTblEntry;

    delay(200); //Give time for address change

    rcode = pUsb->setConf(bAddress, epInfo[ XBOX_CONTROL_PIPE ].epAddr, 1);
    if(rcode)
        goto FailSetConfDescr;

    XboxReceiverConnected = true;
    bPollEnable = true;
    checkStatusTimer = 0; // Reset timer
    return 0; // Successful configuration

    /* Diagnostic messages */
    FailSetDevTblEntry:


    FailSetConfDescr:


    Fail:
    Release();

    return rcode;
}

/* Performs a cleanup after failed Init() attempt */
uint8_t XBOXRECV::Release() {
    XboxReceiverConnected = false;
    for(uint8_t i = 0; i < 4; i++)
    Xbox360Connected[i] = 0x00;

    pUsb->GetAddressPool().FreeAddress(bAddress);
    bAddress = 0;
    bPollEnable = false;
    return 0;
}

uint8_t XBOXRECV::Poll() {
    if(!bPollEnable)
    return 0;

    //Chatpad LEDs are really finicky.
    //I queue them up in a FIFO buffer then slowly process the queue.
    if(millis()-chatPadLedTimer>250){
        for(uint8_t i=0; i<4; i++){
            chatPadProcessLed(i);
        }
        chatPadLedTimer=millis();

        //Windows driver does this every 2.5 seconds. May aswell do the same
        //Polls controller to check if present
        //Requests the controllers battery status
        //Set the LED to the correct quadrant
        } else if(millis()-checkStatusTimer>2500){
        static int8_t pollState=0;
        for (uint8_t i=0;i<4;i++){
            switch (pollState){
                case 0:
                checkControllerPresence(i);
                break;

                case 1:
                if(Xbox360Connected[i]){
                    checkControllerBattery(i);
                    setLedRaw(0x06+i, i);
                }
                break;

                case 2:
                if(Xbox360Connected[i]){
                    setLedRaw(0x06+i, i);
                }
                break;

                case 3:
                chatPadKeepAlive1(i);
                break;

                case 4:
                chatPadKeepAlive2(i);
                break;

                case 5:
                checkStatusTimer=millis();
                pollState=-1;
                break;
            }
        }
        pollState++;
    }

    uint8_t inputPipe;
    uint16_t bufferSize;
    for(uint8_t i = 0; i < 4; i++) {

        //If there is a chatpad installed, we send init
        //packets to it if required.
        if(chatPadInitNeeded[i]){
            enableChatPad(i);
            chatPadInitNeeded[i]=0;
        }

        if(i == 0)
        inputPipe = XBOX_INPUT_PIPE_1;
        else if(i == 1)
        inputPipe = XBOX_INPUT_PIPE_2;
        else if(i == 2)
        inputPipe = XBOX_INPUT_PIPE_3;
        else
        inputPipe = XBOX_INPUT_PIPE_4;


        bufferSize = EP_MAXPKTSIZE; // This is the maximum number of bytes we want to receive
        pUsb->inTransfer(bAddress, epInfo[ inputPipe ].epAddr, &bufferSize, readBuf);
        if(bufferSize > 0) { // The number of received bytes
            readReport(i);
        }
    }
    return 0;

}

void XBOXRECV::readReport(uint8_t controller) {
    if(readBuf == NULL)
    return;
    // This report is sent when a controller is connected and disconnected
    if(readBuf[0] & 0x08 && readBuf[1] != Xbox360Connected[controller]) {
        /*
        readBuf[1]==0x80 for a controller
        readBuf[1]==0x40 for a headset only
        readBuf[1]==0xC0 for a controller+headset only
        */
        Xbox360Connected[controller] = readBuf[1];

        if(Xbox360Connected[controller]) {
            chatPadInitNeeded[controller]=1; //Chatpad init set true by default
            onInit(controller);
            } else {
            chatPadInitNeeded[controller]=0;
        }
        return;
    }

    // Controller status report
    if(readBuf[1] == 0x00 && readBuf[3] & 0x13 && readBuf[4] >= 0x22) {
        controllerStatus[controller] = ((uint16_t)readBuf[3] << 8) | readBuf[4];
        return;
    }

    //Standard controller event
    if(readBuf[0] == 0x00 && readBuf[1] == 0x01){ // Check if it's the correct report - the receiver also sends different status reports
        // A controller must be connected if it's sending data
        if(!Xbox360Connected[controller]){
            Xbox360Connected[controller] |= 0x80;
        }


        ButtonState[controller] = (uint32_t)(readBuf[9] | ((uint16_t)readBuf[8] << 8) | ((uint32_t)readBuf[7] << 16) | ((uint32_t)readBuf[6] << 24));

        hatValue[controller][LeftHatX] = (int16_t)(((uint16_t)readBuf[11] << 8) | readBuf[10]);
        hatValue[controller][LeftHatY] = (int16_t)(((uint16_t)readBuf[13] << 8) | readBuf[12]);
        hatValue[controller][RightHatX] = (int16_t)(((uint16_t)readBuf[15] << 8) | readBuf[14]);
        hatValue[controller][RightHatY] = (int16_t)(((uint16_t)readBuf[17] << 8) | readBuf[16]);

        if(ButtonState[controller] != OldButtonState[controller]) {
            buttonStateChanged[controller] = true;
            ButtonClickState[controller] = (ButtonState[controller] >> 16) & ((~OldButtonState[controller]) >> 16); // Update click state variable, but don't include the two trigger buttons L2 and R2
            if(((uint8_t)OldButtonState[controller]) == 0 && ((uint8_t)ButtonState[controller]) != 0) {
                R2Clicked[controller] = true;
            }

            if((uint8_t)(OldButtonState[controller] >> 8) == 0 && (uint8_t)(ButtonState[controller] >> 8) != 0){
                L2Clicked[controller] = true;
            }

            OldButtonState[controller] = ButtonState[controller];
        }

        //Chatpad Events
        } else if(readBuf[0] == 0x00 && readBuf[1] == 0x02){

        //This s a key press event
        if(readBuf[24] == 0x00){
            ChatPadState[controller]=0;
            ChatPadState[controller]	|=((uint32_t)(readBuf[25])<<16) & 0xFF0000; //This contains modifiers like shift, green, orange and messenger buttons They are OR'd together in one byte
            ChatPadState[controller]	|=((uint32_t)(readBuf[26])<<8)  & 0xFFFF00;  //This contains the first button being pressed.
            ChatPadState[controller]	|=(uint32_t)(readBuf[27]);		//This contains the second button being pressed.

            if(ChatPadState[controller] != OldChatPadState[controller]) {
                ChatPadStateChanged[controller] = true;
                ChatPadClickState[controller] = ChatPadState[controller] & ~OldChatPadState[controller];
                OldChatPadState[controller] = ChatPadState[controller];
            }
            //This is a handshake request
            } else if (readBuf[24] == 0xF0 && readBuf[25] == 0x03){
            chatPadInitNeeded[controller]=1;
        }


    }
    memset(readBuf,0x00,32);
}

uint8_t XBOXRECV::getButtonPress(ButtonEnum b, uint8_t controller) {
    if(b == L2) // These are analog buttons
    return (uint8_t)(ButtonState[controller] >> 8);
    else if(b == R2)
    return (uint8_t)ButtonState[controller];
    return (bool)(ButtonState[controller] & ((uint32_t)pgm_read_word(&XBOX_BUTTONS[(uint8_t)b]) << 16));
}

bool XBOXRECV::getButtonClick(ButtonEnum b, uint8_t controller) {
    if(b == L2) {
        if(L2Clicked[controller]) {
            L2Clicked[controller] = false;
            return true;
        }
        return false;
        } else if(b == R2) {
        if(R2Clicked[controller]) {
            R2Clicked[controller] = false;
            return true;
        }
        return false;
    }
    uint16_t button = pgm_read_word(&XBOX_BUTTONS[(uint8_t)b]);
    bool click = (ButtonClickState[controller] & button);
    ButtonClickState[controller] &= ~button; // clear "click" event
    return click;
}


uint8_t XBOXRECV::getChatPadPress(ChatPadButton b, uint8_t controller) {
    uint8_t button = b;
    uint8_t ChatPadState1=(uint8_t)(ChatPadState[controller]>>16 & 0x0000FF);
    uint8_t ChatPadState2=(uint8_t)(ChatPadState[controller]>>8 & 0x0000FF);
    uint8_t ChatPadState3=(uint8_t)(ChatPadState[controller]>>0 & 0x0000FF);

    if(button<17 && ChatPadState1&button){
        return 1;
        } else if (ChatPadState2==button || ChatPadState3==button){
        return 1;
        } else {
        return 0;
    }
}

uint8_t XBOXRECV::getChatPadClick(ChatPadButton b, uint8_t controller) {
    uint8_t button = b;
    uint8_t ChatPadClickState1=(uint8_t)(ChatPadClickState[controller]>>16 & 0x0000FF);
    uint8_t ChatPadClickState2=(uint8_t)(ChatPadClickState[controller]>>8 & 0x0000FF);
    uint8_t ChatPadClickState3=(uint8_t)(ChatPadClickState[controller]>>0 & 0x0000FF);

    if(button<17 && ChatPadClickState1&button){
        ChatPadClickState[controller] &= ~(((uint32_t)button<<16)&0xFF0000);
        return 1;
        } else if (ChatPadClickState2==button){
        ChatPadClickState[controller]&=0xFF00FF;
        return 1;
        } else if (ChatPadClickState3==button){
        ChatPadClickState[controller]&=0xFFFF00;
        return 1;
        } else {
        return 0;
    }

}

int16_t XBOXRECV::getAnalogHat(AnalogHatEnum a, uint8_t controller) {
    return hatValue[controller][a];
}

bool XBOXRECV::buttonChanged(uint8_t controller) {
    bool state = buttonStateChanged[controller];
    buttonStateChanged[controller] = false;
    return state;
}

/*
ControllerStatus Breakdown
ControllerStatus[controller] & 0x0001   // 0
ControllerStatus[controller] & 0x0002   // normal batteries, no rechargeable battery pack
ControllerStatus[controller] & 0x0004   // controller starting up / settling
ControllerStatus[controller] & 0x0008   // headset adapter plugged in, but no headphones connected (mute?)
ControllerStatus[controller] & 0x0010   // 0
ControllerStatus[controller] & 0x0020   // 1
ControllerStatus[controller] & 0x0040   // battery level (high bit)
ControllerStatus[controller] & 0x0080   // battery level (low bit)
ControllerStatus[controller] & 0x0100   // 1
ControllerStatus[controller] & 0x0200   // 1
ControllerStatus[controller] & 0x0400   // headset adapter plugged in
ControllerStatus[controller] & 0x0800   // 0
ControllerStatus[controller] & 0x1000   // 1
ControllerStatus[controller] & 0x2000   // 0
ControllerStatus[controller] & 0x4000   // 0
ControllerStatus[controller] & 0x8000   // 0
*/
uint8_t XBOXRECV::getBatteryLevel(uint8_t controller) {
    return ((controllerStatus[controller] & 0x00C0) >> 6);
}

void XBOXRECV::XboxCommand(uint8_t controller, uint8_t* data, uint16_t nbytes) {
    static uint32_t outputCommandTimer=0;
    uint8_t outputPipe;
    switch(controller) {
        case 0: outputPipe = XBOX_OUTPUT_PIPE_1;
        break;
        case 1: outputPipe = XBOX_OUTPUT_PIPE_2;
        break;
        case 2: outputPipe = XBOX_OUTPUT_PIPE_3;
        break;
        case 3: outputPipe = XBOX_OUTPUT_PIPE_4;
        break;
        default:
        return;
    }

    while(millis()-outputCommandTimer<1);
    pUsb->outTransfer(bAddress, epInfo[ outputPipe ].epAddr, nbytes, data);
    outputCommandTimer=millis();

}

void XBOXRECV::disconnect(uint8_t controller) {
    memset(writeBuf,0x00,12);
    writeBuf[0] = 0x00;
    writeBuf[1] = 0x00;
    writeBuf[2] = 0x08;
    writeBuf[3] = 0xC0;

    XboxCommand(controller, writeBuf, 12);
}

/*
*  0: off
*  1: all blink, then previous setting
*  2: 1/top-left blink, then on
*  3: 2/top-right blink, then on
*  4: 3/bottom-left blink, then on
*  5: 4/bottom-right blink, then on
*  6: 1/top-left on
*  7: 2/top-right on
*  8: 3/bottom-left on
*  9: 4/bottom-right on
* 10: rotate
* 11: blink, based on previous setting
* 12: slow blink, based on previous setting
* 13: rotate with two lights
* 14: persistent slow all blink
* 15: blink once, then previous setting
*/
void XBOXRECV::setLedRaw(uint8_t value, uint8_t controller) {
    memset(writeBuf,0x00,12);
    writeBuf[0] = 0x00;
    writeBuf[1] = 0x00;
    writeBuf[2] = 0x08;
    writeBuf[3] = value | 0x40;

    XboxCommand(controller, writeBuf, 12);
}

void XBOXRECV::checkControllerPresence(uint8_t controller) {
    memset(writeBuf,0x00,12);
    writeBuf[0] = 0x08;
    writeBuf[1] = 0x00;
    writeBuf[2] = 0x0F;
    writeBuf[3] = 0xc0;
    XboxCommand(controller, writeBuf, 12);

}

void XBOXRECV::checkControllerBattery(uint8_t controller) {
    memset(writeBuf,0x00,12);
    writeBuf[0] = 0x00;
    writeBuf[1] = 0x00;
    writeBuf[2] = 0x00;
    writeBuf[3] = 0x40;
    XboxCommand(controller, writeBuf, 12);
}

void XBOXRECV::enableChatPad(uint8_t controller) {
    memset(writeBuf,0x00,12);
    writeBuf[0] = 0x00;
    writeBuf[1] = 0x00;
    writeBuf[2] = 0x0C;
    writeBuf[3] = 0x1B;
    XboxCommand(controller, writeBuf, 12);
    chatpadEnabled=1;
}


void XBOXRECV::chatPadKeepAlive1(uint8_t controller) {
    memset(writeBuf,0x00,12);
    writeBuf[0] = 0x00;
    writeBuf[1] = 0x00;
    writeBuf[2] = 0x0C;
    writeBuf[3] = 0x1F;
    XboxCommand(controller, writeBuf, 12);
    chatpadEnabled=1;
}

void XBOXRECV::chatPadKeepAlive2(uint8_t controller) {
    memset(writeBuf,0x00,12);
    writeBuf[0] = 0x00;
    writeBuf[1] = 0x00;
    writeBuf[2] = 0x0C;
    writeBuf[3] = 0x1E;
    XboxCommand(controller, writeBuf, 12);
}


void XBOXRECV::setRumbleOn(uint8_t lValue, uint8_t rValue, uint8_t controller) {
    memset(writeBuf,0x00,12);
    writeBuf[0] = 0x00;
    writeBuf[1] = 0x01;
    writeBuf[2] = 0x0f;
    writeBuf[3] = 0xc0;
    writeBuf[4] = 0x00;
    writeBuf[5] = lValue; // big weight
    writeBuf[6] = rValue; // small weight

    XboxCommand(controller, writeBuf, 12);
}

void XBOXRECV::onInit(uint8_t controller) {
    memset(writeBuf,0x00,12);
    setLedRaw(0x00, controller); //Set LED OFF
    delay(8);
    setLedRaw(0x02+controller, controller); //Set LED quadrant blinking

    //Not sure what this is, but windows driver does it
    writeBuf[0] = 0x00;
    writeBuf[1] = 0x00;
    writeBuf[2] = 0x02;
    writeBuf[3] = 0x80;
    XboxCommand(controller, writeBuf, 12);

    //Request battery level
    checkControllerBattery(controller);

    uint8_t inputPipe;
    uint16_t bufferSize;
    if(controller == 0)
    inputPipe = XBOX_INPUT_PIPE_1;
    else if(controller == 1)
    inputPipe = XBOX_INPUT_PIPE_2;
    else if(controller == 2)
    inputPipe = XBOX_INPUT_PIPE_3;
    else
    inputPipe = XBOX_INPUT_PIPE_4;


    bufferSize = EP_MAXPKTSIZE; // This is the maximum number of bytes we want to receive
    pUsb->inTransfer(bAddress, epInfo[ inputPipe ].epAddr, &bufferSize, readBuf);
    pUsb->inTransfer(bAddress, epInfo[ inputPipe ].epAddr, &bufferSize, readBuf);
    delay(2);

    setLedRaw(0x06+controller, controller); //Set LED quadrant on (solid);


}

void XBOXRECV::chatPadQueueLed(uint8_t led, uint8_t controller){
    for(uint8_t i=0;i<4;i++){
        if(chatPadLedQueue[controller][i]==0xFF){
            chatPadLedQueue[controller][i]=led;
            return;
        }
    }
}

void XBOXRECV::chatPadProcessLed(uint8_t controller) {
    if(chatPadLedQueue[controller][0]!=0xFF){
        memset(writeBuf,0x00,12);
        writeBuf[0] = 0x00;
        writeBuf[1] = 0x00;
        writeBuf[2] = 0x0C;
        writeBuf[3] = chatPadLedQueue[controller][0];
        XboxCommand(controller, writeBuf, 12);
        chatPadLedQueue[controller][0]=chatPadLedQueue[controller][1];
        chatPadLedQueue[controller][1]=chatPadLedQueue[controller][2];
        chatPadLedQueue[controller][2]=chatPadLedQueue[controller][3];
        chatPadLedQueue[controller][3]=0xFF;
    }
}