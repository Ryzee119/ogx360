// Copyright 2020, Ryan Wendland, ogx360
// SPDX-License-Identifier: GPL-3.0-or-later

#include "usbh_xinput.h"

#define ENABLE_USBH_XINPUT_DEBUG
#ifdef ENABLE_USBH_XINPUT_DEBUG
#define USBH_XINPUT_DEBUG(a) Serial1.print(a)
#else
#define USBH_XINPUT_DEBUG(...)
#endif

static xinput_pad_t xinput_devices[XINPUT_MAXGAMEPADS];
static xinput_pad_t *pxinput_list = NULL;

static void PrintHex8(uint8_t *data, uint8_t length) // prints 8-bit data in hex with leading zeroes
{
    char tmp[16];
    for (int i = 0; i < length; i++)
    {
        sprintf(tmp, "0x%.2X", data[i]);
        Serial1.print(tmp);
        Serial1.print(" ");
    }
    Serial1.print("\n");
}

xinput_pad_t *XINPUT::alloc_xinput_device(uint8_t bAddress, EpInfo *in, EpInfo *out) {
    xinput_pad_t *new_xinput = NULL;
    for (int i = 0; i < XINPUT_MAXGAMEPADS; i++)
    {
        if (xinput_devices[i].bAddress == 0)
        {
            new_xinput = &xinput_devices[i];
            USBH_XINPUT_DEBUG(F("XINPUT: ALLOCATED NEW XINPUT\n"));
            break;
        }
    }

    if (new_xinput == NULL)
    {
        USBH_XINPUT_DEBUG(F("XINPUT: COULD NOT ALLOCE NEW XINPUT\n"));
        return NULL;
    }

    memset(new_xinput, 0, sizeof(xinput_pad_t));
    new_xinput->bAddress = bAddress;
    new_xinput->usbh_inPipe = in;
    new_xinput->usbh_outPipe = out;
    new_xinput->rValue_requested = 56;
    new_xinput->lValue_requested = 57;
    new_xinput->led_requested = get_xinput_device_index(new_xinput) + 1;

    //Chain the new XID to the end of the list.
    if (pxinput_list == NULL)
    {
        pxinput_list = new_xinput;
    }
    else
    {
        xinput_pad_t *x;
        for (x = pxinput_list; x->next != NULL; x = x->next);
        x->next = new_xinput;
    }

    uint8_t len;
    GetLedCommand(new_xinput, rdata, &len, 0x00);
    pUsb->outTransfer(bAddress, new_xinput->usbh_outPipe->epAddr, len, rdata);

    GetRumbleCommand(new_xinput, rdata, &len, new_xinput->lValue_requested, new_xinput->rValue_requested);
    pUsb->outTransfer(bAddress, new_xinput->usbh_outPipe->epAddr, len, rdata);

    if (xinput_type == XBOX360_WIRELESS)
    {
        memset(rdata,0,12);
        rdata[0] = 0x00;
        rdata[1] = 0x00;
        rdata[2] = 0x02;
        rdata[3] = 0x80;
        pUsb->outTransfer(bAddress, new_xinput->usbh_outPipe->epAddr, 12, rdata);

        memcpy_P(rdata, xbox360w_inquire_present, sizeof(xbox360w_inquire_present));
        pUsb->outTransfer(bAddress, new_xinput->usbh_outPipe->epAddr, sizeof(xbox360w_inquire_present), rdata);

        memcpy_P(rdata, xbox360w_controller_info, sizeof(xbox360w_controller_info));
        pUsb->outTransfer(bAddress, new_xinput->usbh_outPipe->epAddr, sizeof(xbox360w_controller_info), rdata);
    }
    
    return new_xinput;
}

uint8_t XINPUT::free_xinput_device(xinput_pad_t *xinput_dev) {
    //Find the device head in the linked list
    xinput_pad_t *head = pxinput_list;

    //Device is head
    if (xinput_dev == head)
    {
        pxinput_list = pxinput_list->next;
        goto found;
    }

    //Find the head of the device
    while (head != NULL && head->next != xinput_dev)
    {
        head = head->next;
    }

    //Remove it from the linked list
    if (head != NULL)
    {
        xinput_pad_t *new_tail = xinput_dev->next;
        head->next = new_tail;
        goto found;
    }
    return 0;
found:
    memset(xinput_dev, 0, sizeof(xinput_pad_t));
    return 1;
}

uint8_t XINPUT::get_xinput_device_index(xinput_pad_t *xinput) {
    uint8_t index = 0;
    xinput_pad_t *_xinput = usbh_xinput_get_device_list();
    while (_xinput != NULL)
    {
        if (xinput == _xinput)
        {
            break;
        }
        _xinput = _xinput->next;
        index++;
    }
    return index;
}

xinput_pad_t *usbh_xinput_get_device_list(void) {
    return pxinput_list;
}

XINPUT::XINPUT(USB *p) : pUsb(p),
                         bAddress(0),
                         bIsReady(false),
                         timer(0),
                         xinput_type(XINPUT_UNKNOWN)
{
    if (pUsb)
    {
        pUsb->RegisterDeviceClass(this);
    }
}

//FIXME: Hardcoded conf descriptor length. What if its alots larger? Prob dont have enough RAM for this :(
//https://techcommunity.microsoft.com/t5/microsoft-usb-blog/how-does-usb-stack-enumerate-a-device/ba-p/270685
uint8_t conf_desc_buff[64];
uint8_t conf_desc_len = sizeof(conf_desc_buff);
uint8_t XINPUT::ConfigureDevice(uint8_t parent, uint8_t port, bool lowspeed)
{
    uint8_t rcode;
    //Perform some sanity checks of everything
    if (bAddress)
    {
        USBH_XINPUT_DEBUG(F("XINPUT: USB_ERROR_CLASS_INSTANCE_ALREADY_IN_USE"));
        return USB_ERROR_CLASS_INSTANCE_ALREADY_IN_USE;
    }

    AddressPool &addrPool = pUsb->GetAddressPool();
    UsbDevice *p = addrPool.GetUsbDevicePtr(0);

    if (!p)
    {
        USBH_XINPUT_DEBUG(F("XINPUT: USB_ERROR_ADDRESS_NOT_FOUND_IN_POOL"));
        return USB_ERROR_ADDRESS_NOT_FOUND_IN_POOL;
    }

    if (!p->epinfo)
    {
        USBH_XINPUT_DEBUG(F("XINPUT: USB_ERROR_EPINFO_IS_NULL"));
        return USB_ERROR_EPINFO_IS_NULL;
    }

    //Get the device descriptor now, and check its not a standard class driver
    USB_DEVICE_DESCRIPTOR udd;
    p->lowspeed = lowspeed;
    rcode = pUsb->getDevDescr(0, 0, sizeof(USB_DEVICE_DESCRIPTOR), (uint8_t *)(&udd));
    if (rcode)
    {
        USBH_XINPUT_DEBUG(F("XINPUT: getDevDescr error\n"));
        return rcode;
    }

    //Check the device descriptor here. Interface class is checked later.
    if (udd.bDeviceClass != 0xFF && udd.bDeviceClass != 0x00)
    {
        USBH_XINPUT_DEBUG(F("XINPUT: USB_DEV_CONFIG_ERROR_DEVICE_NOT_SUPPORTED. Failed DeviceClass"));
        return USB_DEV_CONFIG_ERROR_DEVICE_NOT_SUPPORTED;
    }

    //Now we have some info for the control pipe
    epInfo[0].epAddr = 0x00;
    epInfo[0].epAttribs = USB_TRANSFER_TYPE_CONTROL;
    epInfo[0].maxPktSize = udd.bMaxPacketSize0;
    epInfo[0].bmNakPower = USB_NAK_MAX_POWER;

    //So far so good, lets issue a reset again, then finish everything off in Init
    return 0;
};

uint8_t XINPUT::Init(uint8_t parent __attribute__((unused)), uint8_t port __attribute__((unused)), bool lowspeed)
{
    uint8_t rcode;

    //Get a USB address then set it
    AddressPool &addrPool = pUsb->GetAddressPool();
    bAddress = addrPool.AllocAddress(parent, false, port);

    if (!bAddress)
    {
        USBH_XINPUT_DEBUG(F("XINPUT: USB_ERROR_OUT_OF_ADDRESS_SPACE_IN_POOL\n"));
        return USB_ERROR_OUT_OF_ADDRESS_SPACE_IN_POOL;
    }

    rcode = pUsb->setAddr(0, 0, bAddress);
    if (rcode)
    {
        USBH_XINPUT_DEBUG(F("XINPUT: setAddr failed\n"));
        Release();
        return rcode;
    }

    delay(10); //Give time for address change

    //Get our new device at the address
    UsbDevice *p = addrPool.GetUsbDevicePtr(bAddress);
    if (!p)
    {
        Release();
        USBH_XINPUT_DEBUG(F("XINPUT: GetUsbDevicePtr error\n"));
        return USB_ERROR_ADDRESS_NOT_FOUND_IN_POOL;
    }
    p->lowspeed = lowspeed;


    //Request the configuration descriptor to determine what xinput device it is and get endpoint info etc.
    rcode = pUsb->getConfDescr(bAddress, 0, conf_desc_len, 0, (uint8_t *)conf_desc_buff);
    if (rcode)
    {
        Release();
        USBH_XINPUT_DEBUG(F("XINPUT: getConfDescr error\n"));
        return rcode;
    }

    //The interface descriptor is after the config descriptor.
    USB_INTERFACE_DESCRIPTOR *uid = reinterpret_cast<USB_INTERFACE_DESCRIPTOR *>(conf_desc_buff +
                                                                                 sizeof(USB_CONFIGURATION_DESCRIPTOR));

    xinput_type = XINPUT_UNKNOWN;
    if (uid->bNumEndpoints < 2)
        xinput_type = XINPUT_UNKNOWN;
    else if (uid->bInterfaceSubClass == 0x5D && //Xbox360 wireless bInterfaceSubClass
             uid->bInterfaceProtocol == 0x81)   //Xbox360 wireless bInterfaceProtocol
        xinput_type = XBOX360_WIRELESS;
    else if (uid->bInterfaceSubClass == 0x5D && //Xbox360 wired bInterfaceSubClass
             uid->bInterfaceProtocol == 0x01)   //Xbox360 wired bInterfaceProtocol
        xinput_type = XBOX360_WIRED;
    else if (uid->bInterfaceSubClass == 0x47 && //Xbone and SX bInterfaceSubClass
             uid->bInterfaceProtocol == 0xD0)   //Xbone and SX bInterfaceProtocol
        xinput_type = XBOXONE;

    if (xinput_type == XINPUT_UNKNOWN)
    {
        Release();
        USBH_XINPUT_DEBUG(F("XINPUT: XINPUT_UNKNOWN\n"));
        return USB_DEV_CONFIG_ERROR_DEVICE_NOT_SUPPORTED;
    }

    USBH_XINPUT_DEBUG(F("Xid type: "));
    USBH_XINPUT_DEBUG(xinput_type);
    USBH_XINPUT_DEBUG("\n");

    for (uint8_t i = 1; i < XBOX_MAX_ENDPOINTS; i++)
    {
        epInfo[i].epAddr = 0x00;
        epInfo[i].epAttribs = USB_TRANSFER_TYPE_INTERRUPT;
        epInfo[i].bmNakPower = USB_NAK_NOWAIT;
        epInfo[i].maxPktSize = EP_MAXPKTSIZE;
        epInfo[i].bmSndToggle = 0;
        epInfo[i].bmRcvToggle = 0;
    }

    if (xinput_type == XBOX360_WIRELESS)
    {
        epInfo[1].epAddr = 0x01;
        epInfo[2].epAddr = 0x01;
        epInfo[3].epAddr = 0x03;
        epInfo[4].epAddr = 0x03;
        epInfo[5].epAddr = 0x05;
        epInfo[6].epAddr = 0x05;
        epInfo[7].epAddr = 0x07;
        epInfo[8].epAddr = 0x07;
    }

    //Parse the configuration descriptor to find the two endpoint addresses (Only used for non wireless receiver)
    uint8_t cd_len = 0, cd_type = 0, cd_pos = 0;
    USB_CONFIGURATION_DESCRIPTOR *ucd = reinterpret_cast<USB_CONFIGURATION_DESCRIPTOR *>(conf_desc_buff);
    while (epInfo[XBOX_INPUT_PIPE].epAddr == 0 || epInfo[XBOX_OUTPUT_PIPE].epAddr == 0)
    {
        if (cd_pos >= sizeof(conf_desc_buff) - 1)
            break;

        cd_len = conf_desc_buff[cd_pos];
        cd_type = conf_desc_buff[cd_pos + 1];

        if (cd_type == USB_ENDPOINT_DESCRIPTOR_TYPE)
        {
            USB_ENDPOINT_DESCRIPTOR *uepd = reinterpret_cast<USB_ENDPOINT_DESCRIPTOR *>(&conf_desc_buff[cd_pos]);
            if (uepd->bmAttributes == USB_TRANSFER_TYPE_INTERRUPT)
            {
                uint8_t pipe = (uepd->bEndpointAddress & 0x80) ? XBOX_INPUT_PIPE : XBOX_OUTPUT_PIPE;
                epInfo[pipe].epAddr = uepd->bEndpointAddress & 0x7F;
            }
        }
        cd_pos += cd_len;
    }

    rcode = pUsb->setEpInfoEntry(bAddress, ((xinput_type == XBOX360_WIRELESS) ? 9 : 3), epInfo);
    if (rcode)
    {
        Release();
        USBH_XINPUT_DEBUG(F("XINPUT: setEpInfoEntry error\n"));
        return rcode;
    }

    //Set the device configuration we want to use
    rcode = pUsb->setConf(bAddress, epInfo[XBOX_CONTROL_PIPE].epAddr, ucd->bConfigurationValue);
    if (rcode)
    {
        Release();
        USBH_XINPUT_DEBUG(F("XINPUT: setConf error\n"));
        return rcode;
    }

    //Get string descriptors!

    bIsReady = true;
    USBH_XINPUT_DEBUG(F("XINPUT ENUMERATED OK!\n"));
    return 0;
}

/* Performs a cleanup after failed Init() attempt */
uint8_t XINPUT::Release()
{
    xinput_pad_t *xinput = usbh_xinput_get_device_list();
    while (xinput != NULL)
    {
        if (xinput->bAddress == bAddress)
        {
            if (free_xinput_device(xinput))
            {
                USBH_XINPUT_DEBUG(F("XINPUT FREED\n"));
                xinput = usbh_xinput_get_device_list();
                continue;
            }  
        }
        xinput = xinput->next;
    }

    pUsb->GetAddressPool().FreeAddress(bAddress);
    memset(epInfo, 0x00, sizeof(EpInfo)*XBOX_MAX_ENDPOINTS);
    bAddress = 0;
    bIsReady = false;
    return 0;
}

uint8_t XINPUT::Poll()
{
    uint16_t len;
    uint8_t rcode;

    if (!bIsReady)
        return 0;

    //Read all endpoints on this device, and parse into the xinput linked list as required.
    for (uint8_t i = 1; i <= ((xinput_type == XBOX360_WIRELESS) ? 8 : 2); i++)
    {
        len = EP_MAXPKTSIZE;

        //Find the xinput struct this endpoint belongs to.
        //If its not yet initialised, it will return NULL.
        xinput_pad_t *xinput = usbh_xinput_get_device_list();
        while (xinput != NULL)
        {
            if (xinput->usbh_inPipe == &epInfo[i] || xinput->usbh_outPipe == &epInfo[i])
                break;
            xinput = xinput->next;
        }

        //The odd numbers are the in endpoints. Need to read always
        if (i % 2 != 0)
            rcode = pUsb->inTransfer(bAddress, epInfo[i].epAddr, &len, rdata);

        if (rcode == hrSUCCESS)
        {
            ParseInputData(&xinput, &epInfo[i]);
        }
        
        while (xinput != NULL)
        {
            //Handle output commands;
            if (xinput->usbh_outPipe == &epInfo[i])
            {
                //Send rumble
                if (xinput->lValue_requested != xinput->lValue_actual || xinput->rValue_requested != xinput->rValue_actual)
                {
                    USBH_XINPUT_DEBUG(F("SET RUMBLE\n"));
                    uint8_t len;
                    GetRumbleCommand(xinput, rdata, &len, xinput->lValue_requested, xinput->rValue_requested);
                    uint8_t rcode = pUsb->outTransfer(bAddress, epInfo[i].epAddr, len, rdata);
                    if (rcode == hrSUCCESS)
                    {
                        xinput->lValue_actual = xinput->lValue_requested;
                        xinput->rValue_actual = xinput->rValue_requested;
                    }
                    else
                    {
                        USBH_XINPUT_DEBUG(F("XINPUT ERROR SENDING RUMBLE COMMAND\n"));
                    }
                }

                //Send led_requested
                else if (xinput->led_requested != xinput->led_actual)
                {
                    USBH_XINPUT_DEBUG(F("SET LED\n"));
                    uint8_t len;
                    GetLedCommand(xinput, rdata, &len, xinput->led_requested);
                    uint8_t rcode = pUsb->outTransfer(bAddress, epInfo[i].epAddr, len, rdata);
                    if (rcode == hrSUCCESS)
                    {
                        xinput->led_actual = xinput->led_requested;
                    }
                    else
                    {
                        USBH_XINPUT_DEBUG(F("XINPUT ERROR SENDING LED COMMAND\n"));
                    }
                }

                else
                {
                    if (millis() - timer > 100)
                    {
                        memcpy_P(rdata, xbox360w_controller_info, sizeof(xbox360w_controller_info));
                        uint8_t rcode = pUsb->outTransfer(bAddress, epInfo[i].epAddr, sizeof(xbox360w_controller_info), rdata);
                        if (rcode != hrSUCCESS)
                        {
                            USBH_XINPUT_DEBUG(F("XINPUT ERROR SENDING PRESNCE COMMAND\n"));
                        }
                        timer = millis();
                    }
                    
                }
            }
            xinput = xinput->next;
        }
    }
    return 0;
}

bool XINPUT::ParseInputData(xinput_pad_t **xpad, EpInfo *ep_in)
{
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wstrict-aliasing"
#define GET_USHORT(a,b) *((uint16_t *)a[b])
#define GET_SHORT(a,b) *((int16_t *)a[b])

    uint16_t wButtons;

    xinput_pad_t *_xpad = *xpad;

    switch(xinput_type)
    {
    case XBOX360_WIRED:

        if (_xpad == NULL)
        {
            USBH_XINPUT_DEBUG(F("XINPUT WIRED NOT ALLOCATED YET. DOING IT NOW\n"));
            _xpad = alloc_xinput_device(bAddress, &ep_in[0], &ep_in[1]);
            if (_xpad == NULL)
                break;
        }

        //Controller led_requested feedback
        if (rdata[0] == 0x01){
            //Convert it to 1-4, 0 for off.
            _xpad->led_actual = (rdata[2] & 0x0F);
            if (_xpad->led_actual != 0)
                _xpad->led_actual -= (_xpad->led_actual > 5) ? 5 : 1;
            //PrintHex8(rdata, 3);
            break;
        }

        //FIXME, What is this?
        else if (rdata[0] == 0x02){
            //PrintHex8(rdata, 8);
            break;
        }

        //Controller rumble feedback
        else if (rdata[0] == 0x03){
            _xpad->lValue_actual = rdata[3] << 8;
            _xpad->rValue_actual = rdata[4] << 8;
            //PrintHex8(rdata, 6);
            break;
        }

        //FIXME, What is this?
        else if (rdata[0] == 0x08){
            //PrintHex8(rdata, 8);
            break;
        }

        else if (rdata[0] != 0x00)
        {
            USBH_XINPUT_DEBUG(F("UNKNOWN XBOX360 WIRED COMMAND\n"));
            break;
        }

        wButtons = GET_USHORT(&rdata, 2);

        //Map digital buttons
        _xpad->pad_state.wButtons = 0x0000;
        if (wButtons & (1 << 0)) _xpad->pad_state.wButtons |= XINPUT_GAMEPAD_DPAD_UP;
        if (wButtons & (1 << 1)) _xpad->pad_state.wButtons |= XINPUT_GAMEPAD_DPAD_DOWN;
        if (wButtons & (1 << 2)) _xpad->pad_state.wButtons |= XINPUT_GAMEPAD_DPAD_LEFT;
        if (wButtons & (1 << 3)) _xpad->pad_state.wButtons |= XINPUT_GAMEPAD_DPAD_RIGHT;
        if (wButtons & (1 << 4)) _xpad->pad_state.wButtons |= XINPUT_GAMEPAD_START;
        if (wButtons & (1 << 5)) _xpad->pad_state.wButtons |= XINPUT_GAMEPAD_BACK;
        if (wButtons & (1 << 6)) _xpad->pad_state.wButtons |= XINPUT_GAMEPAD_LEFT_THUMB;
        if (wButtons & (1 << 7)) _xpad->pad_state.wButtons |= XINPUT_GAMEPAD_RIGHT_THUMB;
        if (wButtons & (1 << 8)) _xpad->pad_state.wButtons |= XINPUT_GAMEPAD_LEFT_SHOULDER;
        if (wButtons & (1 << 9)) _xpad->pad_state.wButtons |= XINPUT_GAMEPAD_RIGHT_SHOULDER;
        if (wButtons & (1 << 12)) _xpad->pad_state.wButtons |= XINPUT_GAMEPAD_A;
        if (wButtons & (1 << 13)) _xpad->pad_state.wButtons |= XINPUT_GAMEPAD_B;
        if (wButtons & (1 << 14)) _xpad->pad_state.wButtons |= XINPUT_GAMEPAD_X;
        if (wButtons & (1 << 15)) _xpad->pad_state.wButtons |= XINPUT_GAMEPAD_Y;

        //Map the left and right triggers
        _xpad->pad_state.bLeftTrigger = rdata[4];
        _xpad->pad_state.bRightTrigger = rdata[5];

        //Map analog sticks
        _xpad->pad_state.sThumbLX = GET_SHORT(&rdata, 6);
        _xpad->pad_state.sThumbLY = GET_SHORT(&rdata, 8);
        _xpad->pad_state.sThumbRX = GET_SHORT(&rdata, 10);
        _xpad->pad_state.sThumbRY = GET_SHORT(&rdata, 12);
        return true;
    case XBOX360_WIRELESS:
        if (rdata[0] & 0x08)
        {
            if (rdata[1] != 0x00 && _xpad == NULL)
            {
                USBH_XINPUT_DEBUG(F("WIRELESS CONTROLLER CONNECTED\n"));
                 _xpad = alloc_xinput_device(bAddress, &ep_in[0], &ep_in[1]);
                if (_xpad == NULL)
                    break;
            }
        }

        PrintHex8(rdata, 20);

        ////Some peripheral is connected to the controller and needs attention. (Should be chatpad)
        if (rdata[1] == 0xF8)
        {

        }

        else if (rdata[1] == 0x00)
        {
            //To tell the host there's no more events?
            break;
        }

        //Controller pad event
        else if ((rdata[1] & 1) && rdata[5] == 0x13)
        {
            wButtons = GET_USHORT(&rdata, 6);
            //Map digital buttons
            _xpad->pad_state.wButtons = 0x0000;
            if (wButtons & (1 << 0)) _xpad->pad_state.wButtons |= XINPUT_GAMEPAD_DPAD_UP;
            if (wButtons & (1 << 1)) _xpad->pad_state.wButtons |= XINPUT_GAMEPAD_DPAD_DOWN;
            if (wButtons & (1 << 2)) _xpad->pad_state.wButtons |= XINPUT_GAMEPAD_DPAD_LEFT;
            if (wButtons & (1 << 3)) _xpad->pad_state.wButtons |= XINPUT_GAMEPAD_DPAD_RIGHT;
            if (wButtons & (1 << 4)) _xpad->pad_state.wButtons |= XINPUT_GAMEPAD_START;
            if (wButtons & (1 << 5)) _xpad->pad_state.wButtons |= XINPUT_GAMEPAD_BACK;
            if (wButtons & (1 << 6)) _xpad->pad_state.wButtons |= XINPUT_GAMEPAD_LEFT_THUMB;
            if (wButtons & (1 << 7)) _xpad->pad_state.wButtons |= XINPUT_GAMEPAD_RIGHT_THUMB;
            if (wButtons & (1 << 8)) _xpad->pad_state.wButtons |= XINPUT_GAMEPAD_LEFT_SHOULDER;
            if (wButtons & (1 << 9)) _xpad->pad_state.wButtons |= XINPUT_GAMEPAD_RIGHT_SHOULDER;
            if (wButtons & (1 << 12)) _xpad->pad_state.wButtons |= XINPUT_GAMEPAD_A;
            if (wButtons & (1 << 13)) _xpad->pad_state.wButtons |= XINPUT_GAMEPAD_B;
            if (wButtons & (1 << 14)) _xpad->pad_state.wButtons |= XINPUT_GAMEPAD_X;
            if (wButtons & (1 << 15)) _xpad->pad_state.wButtons |= XINPUT_GAMEPAD_Y;

            //Map the left and right triggers
            _xpad->pad_state.bLeftTrigger = rdata[8];
            _xpad->pad_state.bRightTrigger = rdata[9];

            //Map analog sticks
            _xpad->pad_state.sThumbLX = GET_SHORT(&rdata, 10);
            _xpad->pad_state.sThumbLY = GET_SHORT(&rdata, 12);
            _xpad->pad_state.sThumbRX = GET_SHORT(&rdata, 14);
            _xpad->pad_state.sThumbRY = GET_SHORT(&rdata, 16);
        }

        else
        {
            //PrintHex8(rdata, 20);
        }
        
        return true;
    case XBOXONE:
        wButtons = GET_USHORT(&rdata, 4);

        //Map digital buttons
        _xpad->pad_state.wButtons = 0x0000;
        if (wButtons & (1 << 8)) _xpad->pad_state.wButtons |= XINPUT_GAMEPAD_DPAD_UP;
        if (wButtons & (1 << 9)) _xpad->pad_state.wButtons |= XINPUT_GAMEPAD_DPAD_DOWN;
        if (wButtons & (1 << 10)) _xpad->pad_state.wButtons |= XINPUT_GAMEPAD_DPAD_LEFT;
        if (wButtons & (1 << 11)) _xpad->pad_state.wButtons |= XINPUT_GAMEPAD_DPAD_RIGHT;
        if (wButtons & (1 << 2)) _xpad->pad_state.wButtons |= XINPUT_GAMEPAD_START;
        if (wButtons & (1 << 3)) _xpad->pad_state.wButtons |= XINPUT_GAMEPAD_BACK;
        if (wButtons & (1 << 14)) _xpad->pad_state.wButtons |= XINPUT_GAMEPAD_LEFT_THUMB;
        if (wButtons & (1 << 15)) _xpad->pad_state.wButtons |= XINPUT_GAMEPAD_RIGHT_THUMB;
        if (wButtons & (1 << 12)) _xpad->pad_state.wButtons |= XINPUT_GAMEPAD_LEFT_SHOULDER;
        if (wButtons & (1 << 13)) _xpad->pad_state.wButtons |= XINPUT_GAMEPAD_RIGHT_SHOULDER;
        if (wButtons & (1 << 4)) _xpad->pad_state.wButtons |= XINPUT_GAMEPAD_A;
        if (wButtons & (1 << 5)) _xpad->pad_state.wButtons |= XINPUT_GAMEPAD_B;
        if (wButtons & (1 << 6)) _xpad->pad_state.wButtons |= XINPUT_GAMEPAD_X;
        if (wButtons & (1 << 7)) _xpad->pad_state.wButtons |= XINPUT_GAMEPAD_Y;

        //Map the left and right triggers
        _xpad->pad_state.bLeftTrigger = rdata[6];
        _xpad->pad_state.bRightTrigger = rdata[8];

        //Map analog sticks
        _xpad->pad_state.sThumbLX = GET_SHORT(&rdata, 10);
        _xpad->pad_state.sThumbLY = GET_SHORT(&rdata, 12);
        _xpad->pad_state.sThumbRX = GET_SHORT(&rdata, 14);
        _xpad->pad_state.sThumbRY = GET_SHORT(&rdata, 16);
        return true;
    default:
        return false;
    }
#pragma GCC diagnostic pop
    return false;
}

bool XINPUT::GetRumbleCommand(xinput_pad_t *xpad, uint8_t *tdata, uint8_t *len, uint8_t lValue, uint8_t rValue)
{
    //Rumble commands for known controllers
    switch (xinput_type)
    {
    case XBOX360_WIRELESS:
        memcpy_P(tdata, xbox360_wireless_rumble, sizeof(xbox360_wireless_rumble));
        tdata[5] = lValue;
        tdata[6] = rValue;
        *len = sizeof(xbox360_wireless_rumble);
        break;
    case XBOX360_WIRED:
        memcpy_P(tdata, xbox360_wired_rumble, sizeof(xbox360_wired_rumble));
        tdata[3] = lValue;
        tdata[4] = rValue;
        *len = sizeof(xbox360_wired_rumble);
        break;
    case XBOXONE:
        memcpy_P(tdata, xbox_one_rumble, sizeof(xbox_one_rumble));
        tdata[8] = lValue / 2.6f;  //Scale is 0 to 100
        tdata[9] = rValue / 2.6f; //Scale is 0 to 100
        *len = sizeof(xbox_one_rumble);
        break;
    default:
        return 0;
    }

    return 1;
}

bool XINPUT::GetLedCommand(xinput_pad_t *xpad, uint8_t *tdata, uint8_t *len, uint8_t quadrant)
{
    //Rumble commands for known controllers
    switch (xinput_type)
    {
    case XBOX360_WIRELESS:
        memcpy_P(tdata, xbox360_wireless_led, sizeof(xbox360_wireless_led));
        tdata[3] = (quadrant == 0) ? 0 : (0x40 | (quadrant + 5));
        *len = sizeof(xbox360_wireless_led);
        break;
    case XBOX360_WIRED:
        memcpy_P(tdata, xbox360_wired_led, sizeof(xbox360_wired_led));
        tdata[2] = (quadrant == 0) ? 0 : (quadrant + 5);
        *len = sizeof(xbox360_wired_led);
        break;
    default:
        return 0;
    }

    return 1;
}