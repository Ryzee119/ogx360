// Copyright 2021, Ryan Wendland, ogx360
// SPDX-License-Identifier: GPL-3.0-or-later

#include <UHS2/usbhid.h>
#include "usbh_xinput.h"

//#define ENABLE_USBH_XINPUT_DEBUG
#ifdef ENABLE_USBH_XINPUT_DEBUG
#define USBH_XINPUT_DEBUG(a) Serial1.print(a)
#else
#define USBH_XINPUT_DEBUG(...)
#endif

//FIXME: This causes cast warnings.
#define GET_USHORT(a, b) *((uint16_t *)a[b])
#define GET_SHORT(a, b) *((int16_t *)a[b])
#define GET_UINT(a, b) *((uint32_t *)a[b])

static usbh_xinput_t xinput_devices[XINPUT_MAXGAMEPADS];
static uint8_t xdata[384];

#ifdef ENABLE_USBH_XINPUT_DEBUG
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
#endif

usbh_xinput_t *XINPUT::alloc_xinput_device(uint8_t bAddress, uint8_t itf_num, EpInfo *in, EpInfo *out, xinput_type_t type)
{
    usbh_xinput_t *new_xinput = NULL;
    uint8_t index;
    for (index = 0; index < XINPUT_MAXGAMEPADS; index++)
    {
        if (xinput_devices[index].bAddress == 0)
        {
            new_xinput = &xinput_devices[index];
            USBH_XINPUT_DEBUG(F("USBH XINPUT: ALLOCATED NEW XINPUT\n"));
            break;
        }
    }

    if (new_xinput == NULL)
    {
        USBH_XINPUT_DEBUG(F("USBH XINPUT: COULD NOT ALLOCATE NEW XINPUT\n"));
        return NULL;
    }

    memset(new_xinput, 0, sizeof(usbh_xinput_t));
    new_xinput->bAddress = bAddress;
    new_xinput->itf_num = itf_num;
    new_xinput->type = type;
    new_xinput->usbh_inPipe = in;
    new_xinput->usbh_outPipe = out;
    new_xinput->led_requested = index + 1;
    new_xinput->chatpad_led_requested = CHATPAD_GREEN;

    if (new_xinput->type == XBOX360_WIRELESS)
    {
        WritePacket(new_xinput, xbox360w_controller_info, sizeof(xbox360w_controller_info), TRANSFER_PGM);
        WritePacket(new_xinput, xbox360w_unknown, sizeof(xbox360w_unknown), TRANSFER_PGM);
        WritePacket(new_xinput, xbox360w_rumble_enable, sizeof(xbox360w_rumble_enable), TRANSFER_PGM);
    }
    else if (new_xinput->type == XBOX360_WIRED)
    {
        uint8_t cmd[sizeof(xbox360_wired_led)];
        memcpy(cmd, xbox360_wired_led, sizeof(xbox360_wired_led));
        cmd[2] = index + 2;
        pUsb->outTransfer(bAddress, out->epAddr, sizeof(xbox360_wired_led), cmd);
    }
    else if (new_xinput->type == XBOXONE)
    {
        WritePacket(new_xinput, xboxone_start_input, sizeof(xboxone_start_input), TRANSFER_PGM);

        //Init packet for XBONE S/Elite controllers (return from bluetooth mode)
        if (VID == 0x045e && (PID == 0x02ea || PID == 0x0b00))
        {
            WritePacket(new_xinput, xboxone_s_init, sizeof(xboxone_s_init), TRANSFER_PGM);
        }

        //Required for PDP aftermarket controllers
        if (VID == 0x0e6f)
        {
            WritePacket(new_xinput, xboxone_pdp_init1, sizeof(xboxone_pdp_init1), TRANSFER_PGM);
            WritePacket(new_xinput, xboxone_pdp_init2, sizeof(xboxone_pdp_init2), TRANSFER_PGM);
            WritePacket(new_xinput, xboxone_pdp_init3, sizeof(xboxone_pdp_init3), TRANSFER_PGM);
        }
        
        //Required for PowerA aftermarket controllers
        if (VID == 0x24c6)
        {
            WritePacket(new_xinput, xboxone_powera_init1, sizeof(xboxone_powera_init1), TRANSFER_PGM);
            WritePacket(new_xinput, xboxone_powera_init2, sizeof(xboxone_powera_init2), TRANSFER_PGM);
        }
    }
    else if (new_xinput->type == XINPUT_MOUSE || new_xinput->type == XINPUT_KEYBOARD)
    {
        //Set to BOOT protocol
        pUsb->ctrlReq(bAddress,
                      0,                        //ep
                      bmREQ_HID_OUT,            //bmReqType
                      HID_REQUEST_SET_PROTOCOL, //bRequest
                      USB_HID_BOOT_PROTOCOL,    //wValLo
                      0x00,                     //wValHi
                      new_xinput->itf_num,      //wIndex
                      0x0000, 0x0000, NULL, NULL);
    }

    return new_xinput;
}

uint8_t XINPUT::free_xinput_device(usbh_xinput_t *xinput)
{
    uint8_t index;
    for (index = 0; index < XINPUT_MAXGAMEPADS; index++)
    {
        if (&xinput_devices[index] == xinput)
        {
            memset(xinput, 0, sizeof(usbh_xinput_t));
            USBH_XINPUT_DEBUG(F("USBH XINPUT: FREED XINPUT\n"));
            return 1;
        }
    }
    return 0;
}

usbh_xinput_t *usbh_xinput_get_device_list(void)
{
    return xinput_devices;
}

uint8_t usbh_xinput_is_chatpad_pressed(usbh_xinput_t *xinput, uint16_t code)
{
    if (xinput->bAddress == 0)
        return 0;

    if (code < 17 && (xinput->chatpad_state[0] & code))
        return 1;

    if (code < 17)
        return 0;

    if (xinput->chatpad_state[1] == code)
        return 1;

    if (xinput->chatpad_state[2] == code)
        return 1;

    return 0;
}

uint8_t usbh_xinput_was_chatpad_pressed(usbh_xinput_t *xinput, uint16_t code)
{
    //Button isnt pressed anymore, Clear it from the history
    if (usbh_xinput_is_chatpad_pressed(xinput, code) == 0)
    {
        for (uint8_t i = 0; i < 3; i++)
        {
            if (xinput->chatpad_state_old[i] == code)
                xinput->chatpad_state_old[i] = 0x00;
        }
        return 0;
    }

    //Button is pressed, and hasnt been pressed before
    if (xinput->chatpad_state_old[0] != code &&
        xinput->chatpad_state_old[1] != code &&
        xinput->chatpad_state_old[2] != code)
    {
        for (uint8_t i = 0; i < 3; i++)
        {
            if (xinput->chatpad_state_old[i] == 0)
            {
                xinput->chatpad_state_old[i] = code;
                return 1;
            }
        }
    }
    return 0;
}

uint8_t usbh_xinput_is_gamepad_pressed(usbh_xinput_t *xinput, uint16_t button_mask)
{
    if (xinput->bAddress == 0)
        return 0;

    return xinput->pad_state.wButtons & button_mask;
}

uint8_t usbh_xinput_was_gamepad_pressed(usbh_xinput_t *xinput, uint16_t button_mask)
{
    if (usbh_xinput_is_gamepad_pressed(xinput, button_mask))
    {
        if ((xinput->pad_state_wButtons_old & button_mask) == 0)
        {
            xinput->pad_state_wButtons_old |= button_mask;
            return 1;
        }
    }
    else
    {
        xinput->pad_state_wButtons_old &= ~button_mask;
    }
    return 0;
}

XINPUT::XINPUT(USB *p) : pUsb(p),
                         bAddress(0),
                         bIsReady(false),
                         PID(0), VID(0),
                         dev_num_eps(1)
{
    memset(xdata, 0x00, sizeof(xdata));
    if (pUsb)
    {
        pUsb->RegisterDeviceClass(this);
    }
}

uint8_t XINPUT::Init(uint8_t parent __attribute__((unused)), uint8_t port __attribute__((unused)), bool lowspeed, USB_DEVICE_DESCRIPTOR* udd)
{
    uint8_t rcode;
    AddressPool &addrPool = pUsb->GetAddressPool();
    UsbDevice *p;
    dev_num_eps = 1;
    iProduct = 0;
    dev_type = XINPUT_UNKNOWN;
    bIsReady = false;

    //Perform some sanity checks of everything
    if (bAddress)
    {
        USBH_XINPUT_DEBUG(F("USBH XINPUT: USB_ERROR_CLASS_INSTANCE_ALREADY_IN_USE\n"));
        return USB_ERROR_CLASS_INSTANCE_ALREADY_IN_USE;
    }

    epInfo[XBOX_CONTROL_PIPE].epAddr = 0x00;
    epInfo[XBOX_CONTROL_PIPE].epAttribs = USB_TRANSFER_TYPE_CONTROL;
    epInfo[XBOX_CONTROL_PIPE].maxPktSize = udd->bMaxPacketSize0;
    epInfo[XBOX_CONTROL_PIPE].bmNakPower = USB_NAK_MAX_POWER;
    pUsb->setEpInfoEntry(bAddress, 1, epInfo);

    for (uint8_t i = 1; i < XBOX_MAX_ENDPOINTS; i++)
    {
        epInfo[i].epAddr = 0x00;
        epInfo[i].epAttribs = USB_TRANSFER_TYPE_INTERRUPT;
        epInfo[i].bmNakPower = USB_NAK_NOWAIT;
        epInfo[i].bmSndToggle = 0;
        epInfo[i].bmRcvToggle = 0;
    }

    //Get a USB address then set it
    bAddress = addrPool.AllocAddress(parent, false, port);
    if (!bAddress)
    {
        USBH_XINPUT_DEBUG(F("USBH XINPUT: USB_ERROR_OUT_OF_ADDRESS_SPACE_IN_POOL\n"));
        return USB_ERROR_OUT_OF_ADDRESS_SPACE_IN_POOL;
    }

    rcode = pUsb->setAddr(0, XBOX_CONTROL_PIPE, bAddress);
    if (rcode)
    {
        USBH_XINPUT_DEBUG(F("USBH XINPUT: setAddr failed\n"));
        Release();
        return rcode;
    }

    delay(20); //Give time for address change

    //Get our new device at the address
    p = addrPool.GetUsbDevicePtr(bAddress);
    if (!p)
    {
        Release();
        USBH_XINPUT_DEBUG(F("USBH XINPUT: GetUsbDevicePtr error\n"));
        return USB_ERROR_ADDRESS_NOT_FOUND_IN_POOL;
    }

    p->lowspeed = lowspeed;

    PID = udd->idProduct;
    VID = udd->idVendor;

    iProduct = udd->iProduct;
    iManuf = udd->iManufacturer;
    iSerial = udd->iSerialNumber;

    //Get the device descriptor at the new address
    rcode = pUsb->getDevDescr(bAddress, 0, sizeof(USB_DEVICE_DESCRIPTOR), xdata);
    if (rcode)
    {
            return rcode;
    }
    //Request the first 9bytes of the configuration descriptor to determine the max length
    USB_CONFIGURATION_DESCRIPTOR *ucd = reinterpret_cast<USB_CONFIGURATION_DESCRIPTOR *>(xdata);
    rcode = pUsb->getConfDescr(bAddress, XBOX_CONTROL_PIPE, 9, 0, xdata);
    if (rcode)
    {
        Release();
        USBH_XINPUT_DEBUG(F("USBH XINPUT: getConfDescr error\n"));
        return rcode;
    }
    //Request the full configuration descriptor to determine what xinput device it is and get endpoint info etc.
    rcode = pUsb->getConfDescr(bAddress, XBOX_CONTROL_PIPE, ucd->wTotalLength, 0, xdata);
    if (rcode)
    {
        Release();
        USBH_XINPUT_DEBUG(F("USBH XINPUT: getConfDescr error\n"));
        return rcode;
    }
    //Set the device configuration we want to use
    rcode = pUsb->setConf(bAddress, epInfo[XBOX_CONTROL_PIPE].epAddr, ucd->bConfigurationValue);
    if (rcode)
    {
        Release();
        USBH_XINPUT_DEBUG(F("USBH XINPUT: setConf error\n"));
        return rcode;
    }

    uint8_t num_itf = ucd->bNumInterfaces;
    uint8_t *pdesc = (uint8_t *)ucd;
    uint8_t pdesc_pos = 0;
    USB_INTERFACE_DESCRIPTOR *uid;
    while (num_itf)
    {
        //Find next interface
        while (pdesc[1] != USB_DESCRIPTOR_INTERFACE)
        {
            //Shift the pointer by bLength
            pdesc_pos += pdesc[0];
            if (pdesc_pos > sizeof(xdata) || pdesc_pos > ucd->wTotalLength)
            {
                USBH_XINPUT_DEBUG(F("USBH XINPUT: BUFFER OVERLOW PARSING INTERFACES\n"));
                return hrBADREQ;
            }
            pdesc += pdesc[0];
        }
        uid = reinterpret_cast<USB_INTERFACE_DESCRIPTOR *>(pdesc);
        xinput_type_t _type = XINPUT_UNKNOWN;
        if (uid->bNumEndpoints < 1)
            _type = XINPUT_UNKNOWN;
        else if (uid->bInterfaceSubClass == 0x5D && //Xbox360 wireless bInterfaceSubClass
                uid->bInterfaceProtocol == 0x81)   //Xbox360 wireless bInterfaceProtocol
            _type = XBOX360_WIRELESS;
        else if (uid->bInterfaceSubClass == 0x5D && //Xbox360 wired bInterfaceSubClass
                uid->bInterfaceProtocol == 0x01)   //Xbox360 wired bInterfaceProtocol
            _type = XBOX360_WIRED;
        else if (uid->bInterfaceSubClass == 0x47 && //Xbone and SX bInterfaceSubClass
                uid->bInterfaceProtocol == 0xD0)   //Xbone and SX bInterfaceProtocol
            _type = XBOXONE;
        else if (uid->bInterfaceClass == 0x58 &&  //XboxOG bInterfaceClass
                uid->bInterfaceSubClass == 0x42) //XboxOG bInterfaceSubClass
            _type = XBOXOG;
        else if (uid->bInterfaceClass == USB_CLASS_HID &&
                 uid->bInterfaceSubClass == 1 && //Supports boot protocol
                uid->bInterfaceProtocol  == USB_HID_PROTOCOL_KEYBOARD)
            _type = XINPUT_KEYBOARD;
        else if (uid->bInterfaceClass == USB_CLASS_HID &&
                uid->bInterfaceSubClass == 1 && //Supports boot protocol
                uid->bInterfaceProtocol  == USB_HID_PROTOCOL_MOUSE)
            _type = XINPUT_MOUSE;
        else if (uid->bInterfaceClass == USB_CLASS_HID &&
                uid->bInterfaceSubClass == 0 && //Supports boot protocol
                uid->bInterfaceProtocol  == USB_HID_PROTOCOL_NONE &&
                VID == 0x2DC8)
            _type = XINPUT_8BITDO_IDLE;

        if (_type == XINPUT_UNKNOWN)
        {
            num_itf--;
            pdesc += pdesc[0];
            continue;
        }

        if (_type == XBOXONE)
        {
            //For XBONE we only want the first interface
            num_itf = 1;
        }

        USBH_XINPUT_DEBUG(F("USBH XINPUT: XID TYPE: "));
        USBH_XINPUT_DEBUG(_type);
        USBH_XINPUT_DEBUG("\n");

        //Parse the configuration descriptor to find the two endpoint addresses (Only used for non wireless receiver)
        uint8_t cd_len = 0, cd_type = 0, cd_pos = 0, ep_num = 0;
        EpInfo *ep_in, *ep_out = NULL;
        while (ep_num < uid->bNumEndpoints)
        {
            if (cd_pos >= sizeof(xdata) - 1)
                break;

            uint8_t *pepdesc = (uint8_t *)uid;

            cd_len = pepdesc[cd_pos];
            cd_type = pepdesc[cd_pos + 1];
            if (cd_type == USB_ENDPOINT_DESCRIPTOR_TYPE)
            {
                USB_ENDPOINT_DESCRIPTOR *uepd = reinterpret_cast<USB_ENDPOINT_DESCRIPTOR *>(&pdesc[cd_pos]);
                if (uepd->bmAttributes == USB_TRANSFER_TYPE_INTERRUPT)
                {
                    uint8_t pipe = ep_num + dev_num_eps; //Register it after any previous endpoints
                    epInfo[pipe].epAddr = uepd->bEndpointAddress & 0x7F;
                    epInfo[pipe].maxPktSize = uepd->wMaxPacketSize & 0xFF;
                    epInfo[pipe].dir = uepd->bEndpointAddress & 0x80;
                    if (uepd->bEndpointAddress & 0x80)
                    {
                        ep_in = &epInfo[pipe];
                    }
                    else
                    {
                        ep_out = &epInfo[pipe];
                    }
                }
                ep_num++;
            }
            cd_pos += cd_len;
        }

        dev_num_eps += uid->bNumEndpoints;
        //Update the device EP table with the latest interface
        rcode = pUsb->setEpInfoEntry(bAddress, dev_num_eps, epInfo);
        if (rcode)
        {
            Release();
            USBH_XINPUT_DEBUG(F("USBH XINPUT: setEpInfoEntry error\n"));
            return rcode;
        }

        //Wired we can allocate immediately.
        if (_type != XBOX360_WIRELESS)
        {
            alloc_xinput_device(bAddress, uid->bInterfaceNumber, ep_in, ep_out, _type);
        }
        else
        {
            //For the wireless controller send an inquire packet to each endpoint (Even endpoints only)
            dev_type = XBOX360_WIRELESS;
            uint8_t cmd[sizeof(xbox360w_inquire_present)];
            memcpy_P(cmd, xbox360w_inquire_present, sizeof(xbox360w_inquire_present));
            pUsb->outTransfer(bAddress, ep_out->epAddr, sizeof(xbox360w_inquire_present), cmd);
        }
        num_itf--;
        pdesc += pdesc[0];
    }

    //Hack, Retroflag controller needs a product string request on enumeration to work.
    if (iProduct)
    {
        rcode = pUsb->getStrDescr(bAddress, epInfo[XBOX_CONTROL_PIPE].epAddr, 2, iProduct, 0x0409, xdata);
        if (rcode == hrSUCCESS && xdata[1] == USB_DESCRIPTOR_STRING)
        {
            pUsb->getStrDescr(bAddress, epInfo[XBOX_CONTROL_PIPE].epAddr, min(xdata[0], sizeof(xdata)), iProduct, 0x0409, xdata);
        }
    }

    USBH_XINPUT_DEBUG(F("USBH XINPUT: Found valid EPs "));
    USBH_XINPUT_DEBUG(dev_num_eps);
    USBH_XINPUT_DEBUG("\n");
    if (dev_num_eps < 2)
    {
        Release();
        USBH_XINPUT_DEBUG(F("USBH XINPUT: NO VALID XINPUTS\n"));
        return USB_DEV_CONFIG_ERROR_DEVICE_NOT_SUPPORTED;
    }

    bIsReady = true;
    USBH_XINPUT_DEBUG(F("USBH XINPUT: ENUMERATED OK!\n"));
    return hrSUCCESS;
}

uint8_t XINPUT::Release()
{
    uint8_t index;
    for (index = 0; index < XINPUT_MAXGAMEPADS; index++)
    {
        if (bAddress && xinput_devices[index].bAddress == bAddress)
        {
            memset(&xinput_devices[index], 0x00, sizeof(usbh_xinput_t));
            USBH_XINPUT_DEBUG(F("USBH XINPUT: FREED XINPUT\n"));
        }
    }

    pUsb->GetAddressPool().FreeAddress(bAddress);
    memset(epInfo, 0x00, sizeof(EpInfo) * XBOX_MAX_ENDPOINTS);
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
    for (uint8_t i = 1; i < dev_num_eps; i++)
    {
        //Find the xinput struct this endpoint belongs to. If its not yet initialised, it will return NULL.
        usbh_xinput_t *xinput = NULL;
        for (uint8_t index = 0; index < XINPUT_MAXGAMEPADS; index++)
        {
            if (xinput_devices[index].bAddress == 0)
            {
                continue;
            }

            if (xinput_devices[index].usbh_inPipe == &epInfo[i] || xinput_devices[index].usbh_outPipe == &epInfo[i])
            {
                xinput = &xinput_devices[index];
                break;
            }
        }

        //Read the in endpoints. For xbox wireless, the controller may not be allocated yet, so
        //read on all odd endpoints too.
        if (epInfo[i].dir & 0x80)
        {
            len = min(epInfo[i].maxPktSize, EP_MAXPKTSIZE);
            uint8_t epaddr = (xinput == NULL) ? epInfo[i].epAddr : xinput->usbh_inPipe->epAddr;
            rcode = pUsb->inTransfer(bAddress, epaddr, &len, xdata);
            if (rcode == hrSUCCESS)
            {
                ParseInputData(&xinput, &epInfo[i]);
            }
            //This is an in pipe, so we're done in this loop.
            continue;
        }

        //Controller isn't connected
        if (xinput == NULL)
        {
            continue;
        }

        //Don't spam output. 20ms is ok for now. (FIXME: Should be bInterval)
        if (millis() - xinput->timer_out < 20)
        {
            continue;
        }

        if (xinput->usbh_outPipe->epAddr == 0x00)
        {
            continue;
        }

        //Send rumble
        if (xinput->lValue_requested != xinput->lValue_actual || xinput->rValue_requested != xinput->rValue_actual)
        {
            USBH_XINPUT_DEBUG(F("SET RUMBLE\n"));
            SetRumble(xinput, xinput->lValue_requested, xinput->rValue_requested);
        }

        //Send LED commands
        else if (xinput->led_requested != xinput->led_actual)
        {
            USBH_XINPUT_DEBUG(F("USBH XINPUT: SET LED\n"));
            SetLed(xinput, xinput->led_requested);
        }

        //Handle chatpad initialisation (Wireless 360 controller only)
        else if (xinput->type == XBOX360_WIRELESS && xinput->chatpad_initialised == 0)
        {
            USBH_XINPUT_DEBUG(F("USBH XINPUT: SENDING CHATPAD INIT PACKET\n"));
            WritePacket(xinput, xbox360w_chatpad_init, sizeof(xbox360w_chatpad_init), TRANSFER_PGM);
            xinput->chatpad_initialised = 1;
        }

        //Handle chatpad leds (Wireless 360 controller only)
        else if (xinput->type == XBOX360_WIRELESS && xinput->chatpad_led_requested != xinput->chatpad_led_actual)
        {
            memcpy_P(xdata, xbox360w_chatpad_led_ctrl, sizeof(xbox360w_chatpad_led_ctrl));
            for (uint8_t led = 0; led < 4; led++)
            {
                uint8_t actual = xinput->chatpad_led_actual & pgm_read_byte(&chatpad_mod[led]);
                uint8_t want = xinput->chatpad_led_requested & pgm_read_byte(&chatpad_mod[led]);
                //The user has requested a led to turn on that isnt already on
                if (!actual && want)
                {
                    xdata[3] = pgm_read_byte(&chatpad_led_on[led]);
                    xinput->chatpad_led_actual |= pgm_read_byte(&chatpad_mod[led]);
                }
                //The user has requested a led to turn off that isnt already off
                else if (actual && !want)
                {
                    xdata[3] = pgm_read_byte(&chatpad_led_off[led]);
                    xinput->chatpad_led_actual &= ~pgm_read_byte(&chatpad_mod[led]);
                }
                else
                {
                    //No change, check next chatpad led
                    continue;
                }
                USBH_XINPUT_DEBUG(F("USBH XINPUT: SET CHATPAD LED\n"));
                pUsb->outTransfer(bAddress, epInfo[i].epAddr, sizeof(xbox360w_chatpad_led_ctrl), xdata);
                xinput->timer_out = millis();
                xinput->timer_periodic -= 2000; //Force chatpad keep alive packet check
            }
        }

        //Handle controller power off, Hold Xbox button. (Wireless 360 controller only)
        else if (xinput->type == XBOX360_WIRELESS && xinput->pad_state.wButtons & XINPUT_GAMEPAD_XBOX_BUTTON)
        {
            if ((millis() - xinput->timer_poweroff) > 1000)
            {
                USBH_XINPUT_DEBUG(F("USBH XINPUT: POWERING OFF CONTROLLER\n"));
                WritePacket(xinput, xbox360w_power_off, sizeof(xbox360w_power_off), TRANSFER_PGM);
                xinput->timer_poweroff = millis();
            }
        }

        //Reset xbox button hold timer (Wireless 360 only)
        if (xinput->type == XBOX360_WIRELESS && !(xinput->pad_state.wButtons & XINPUT_GAMEPAD_XBOX_BUTTON))
        {
            xinput->timer_poweroff = millis();
        }

        //Handle background periodic writes
        if (millis() - xinput->timer_periodic > 1000)
        {
            //USBH_XINPUT_DEBUG(F("USBH XINPUT: BACKGROUND POLL\n"));
            if (xinput->type == XBOX360_WIRELESS)
            {
                WritePacket(xinput, xbox360w_inquire_present, sizeof(xbox360w_inquire_present), TRANSFER_PGM);
                WritePacket(xinput, xbox360w_controller_info, sizeof(xbox360w_controller_info), TRANSFER_PGM);
                SetLed(xinput, xinput->led_requested);
                (xinput->chatpad_keepalive_toggle ^= 1) ? \
                    WritePacket(xinput, xbox360w_chatpad_keepalive1, sizeof(xbox360w_chatpad_keepalive1), TRANSFER_PGM) :
                    WritePacket(xinput, xbox360w_chatpad_keepalive2, sizeof(xbox360w_chatpad_keepalive2), TRANSFER_PGM);
            }
            xinput->timer_periodic = millis();
        }
    }

    return 0;
}

bool XINPUT::ParseInputData(usbh_xinput_t **xpad, EpInfo *ep_in)
{
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wstrict-aliasing"

    uint16_t wButtons;
    usbh_xinput_t *_xpad = *xpad;

    //Wireless controllers havent been allocated yet, work out the type at the device level.
    xinput_type_t _dev_type = (dev_type == XBOX360_WIRELESS) ? dev_type : _xpad->type;
    switch(_dev_type)
    {
    case XBOX360_WIRED:
        //Controller led_requested feedback
        if (xdata[0] == 0x01){
            //Convert it to 1-4, 0 for off.
            _xpad->led_actual = (xdata[2] & 0x0F);
            if (_xpad->led_actual != 0)
                _xpad->led_actual -= (_xpad->led_actual > 5) ? 5 : 1;
            break;
        }

        //Controller rumble feedback
        else if (xdata[0] == 0x03){
            _xpad->lValue_actual = xdata[3] << 8;
            _xpad->rValue_actual = xdata[4] << 8;
            break;
        }

#if (0)
        //FIXME, What is this? Happens on connection, I get:
        //0x02 0x03 0x00
        else if (xdata[0] == 0x02){
            PrintHex8(xdata,12);
            break;
        }

        //FIXME, What is this? Happens on connection, I get:
        //0x08 0x03 0x00
        else if (xdata[0] == 0x08)
        {
            break;
        }
#endif

        else if (xdata[0] != 0x00)
        {
            USBH_XINPUT_DEBUG(F("USBH XINPUT: UNKNOWN XBOX360 WIRED COMMAND\n"));
            break;
        }

        if(xdata[1] != 0x14)
        {
            break;
        }

        wButtons = GET_USHORT(&xdata, 2);

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
        _xpad->pad_state.bLeftTrigger = xdata[4];
        _xpad->pad_state.bRightTrigger = xdata[5];

        //Map analog sticks
        _xpad->pad_state.sThumbLX = GET_SHORT(&xdata, 6);
        _xpad->pad_state.sThumbLY = GET_SHORT(&xdata, 8);
        _xpad->pad_state.sThumbRX = GET_SHORT(&xdata, 10);
        _xpad->pad_state.sThumbRY = GET_SHORT(&xdata, 12);
        return true;
    case XBOX360_WIRELESS:
        if (xdata[0] & 0x08)
        {
            //Connected packet
            if (xdata[1] != 0x00 && _xpad == NULL)
            {
                USBH_XINPUT_DEBUG(F("USBH XINPUT: WIRELESS CONTROLLER CONNECTED\n"));
                 _xpad = alloc_xinput_device(bAddress, 0, &ep_in[0], &ep_in[1], XBOX360_WIRELESS);
                if (_xpad == NULL)
                    break;
            }
            //Disconnected packet
            else if (xdata[1] == 0x00 && _xpad != NULL)
            {
                free_xinput_device(_xpad);
                _xpad = NULL;
            }
        }

        //If you get to here and the controller still isnt allocated, leave!
        if (_xpad == NULL)
        {
            break;
        }

        //Not sure, seems like I need to send chatpad init packets
        if (xdata[1] == 0xF8)
        {
            USBH_XINPUT_DEBUG("USBH XINPUT: CHATPAD INIT NEEDED1\n");
            _xpad->chatpad_initialised = 0;
        }

        //Controller pad event
        if ((xdata[1] & 1) && xdata[5] == 0x13)
        {
            wButtons = GET_USHORT(&xdata, 6);
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
            if (wButtons & (1 << 10)) _xpad->pad_state.wButtons |= XINPUT_GAMEPAD_XBOX_BUTTON;
            if (wButtons & (1 << 12)) _xpad->pad_state.wButtons |= XINPUT_GAMEPAD_A;
            if (wButtons & (1 << 13)) _xpad->pad_state.wButtons |= XINPUT_GAMEPAD_B;
            if (wButtons & (1 << 14)) _xpad->pad_state.wButtons |= XINPUT_GAMEPAD_X;
            if (wButtons & (1 << 15)) _xpad->pad_state.wButtons |= XINPUT_GAMEPAD_Y;

            //Map the left and right triggers
            _xpad->pad_state.bLeftTrigger = xdata[8];
            _xpad->pad_state.bRightTrigger = xdata[9];

            //Map analog sticks
            _xpad->pad_state.sThumbLX = GET_SHORT(&xdata, 10);
            _xpad->pad_state.sThumbLY = GET_SHORT(&xdata, 12);
            _xpad->pad_state.sThumbRX = GET_SHORT(&xdata, 14);
            _xpad->pad_state.sThumbRY = GET_SHORT(&xdata, 16);
        }

        //Chatpad report
        if ((xdata[1] & 2))
        {
            //Chatpad Button data
            if(xdata[24] == 0x00)
            {
                for (uint8_t i = 0; i < 3; i++)
                {
                    _xpad->chatpad_state[i] = xdata[25 + i];
                }
            }

            //Chatpad Status packet
            if (xdata[24] == 0xF0)
            {
                if (xdata[25] == 0x03)
                {
                    USBH_XINPUT_DEBUG("USBH XINPUT: CHATPAD INIT NEEDED2\n");
                    _xpad->chatpad_initialised = 0;
                }
                //LED status
                if (xdata[25] == 0x04)
                {
                    if (xdata[26] & 0x80)
                    {
                        _xpad->chatpad_led_actual = xdata[26] & 0x7F;
                    }
                }
            }
        }

        return true;
    case XBOXONE:
        if (xdata[0] != 0x20)
        {
            break;
        }

        wButtons = GET_USHORT(&xdata, 4);

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
        _xpad->pad_state.bLeftTrigger = GET_USHORT(&xdata, 6) >> 2;
        _xpad->pad_state.bRightTrigger = GET_USHORT(&xdata, 8) >> 2;

        //Map analog sticks
        _xpad->pad_state.sThumbLX = GET_SHORT(&xdata, 10);
        _xpad->pad_state.sThumbLY = GET_SHORT(&xdata, 12);
        _xpad->pad_state.sThumbRX = GET_SHORT(&xdata, 14);
        _xpad->pad_state.sThumbRY = GET_SHORT(&xdata, 16);
        return true;
    case XBOXOG:
        if (xdata[1] != 0x14)
        {
            break;
        }
  
        wButtons = GET_USHORT(&xdata, 2);

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

        if (xdata[4] > 0x20) _xpad->pad_state.wButtons |= XINPUT_GAMEPAD_A;
        if (xdata[5] > 0x20) _xpad->pad_state.wButtons |= XINPUT_GAMEPAD_B;
        if (xdata[6] > 0x20) _xpad->pad_state.wButtons |= XINPUT_GAMEPAD_X;
        if (xdata[7] > 0x20) _xpad->pad_state.wButtons |= XINPUT_GAMEPAD_Y;
        if (xdata[8] > 0x20) _xpad->pad_state.wButtons |= XINPUT_GAMEPAD_RIGHT_SHOULDER;
        if (xdata[9] > 0x20) _xpad->pad_state.wButtons |= XINPUT_GAMEPAD_LEFT_SHOULDER;

        //Map the left and right triggers
        _xpad->pad_state.bLeftTrigger = xdata[10];
        _xpad->pad_state.bRightTrigger = xdata[11];

        //Map analog sticks
        _xpad->pad_state.sThumbLX = GET_SHORT(&xdata, 12);
        _xpad->pad_state.sThumbLY = GET_SHORT(&xdata, 14);
        _xpad->pad_state.sThumbRX = GET_SHORT(&xdata, 16);
        _xpad->pad_state.sThumbRY = GET_SHORT(&xdata, 18);   
        return true;
    case XINPUT_KEYBOARD:
        Serial1.println("KB: ");
        //PrintHex8(xdata, 8);
        return true;
    case XINPUT_MOUSE:
        Serial1.println("MS: ");
        //PrintHex8(xdata, 8);
        return true;
    default:
        return false;
    }
#pragma GCC diagnostic pop
    return false;
}

uint8_t XINPUT::WritePacket(usbh_xinput_t *xpad, const uint8_t *data, uint8_t len, uint8_t flags)
{
    xpad->timer_out = millis();
    if (flags & TRANSFER_PGM)
    {
        memcpy_P(xdata, data, len);
    }
    else
    {
        memcpy(xdata, data, len);
    }

    return pUsb->outTransfer(bAddress, xpad->usbh_outPipe->epAddr, len, xdata);
}

uint8_t XINPUT::SetRumble(usbh_xinput_t *xpad, uint8_t lValue, uint8_t rValue)
{
    xpad->lValue_actual = xpad->lValue_requested;
    xpad->rValue_actual = xpad->rValue_requested;
    xpad->timer_out = millis();
    uint16_t len;
    switch (xpad->type)
    {
    case XBOX360_WIRELESS:
        memset(xdata, 0x00, 12);
        memcpy_P(xdata, xbox360w_rumble, sizeof(xbox360w_rumble));
        xdata[5] = lValue;
        xdata[6] = rValue;
        len = 12;
        break;
    case XBOX360_WIRED:
        memcpy_P(xdata, xbox360_wired_rumble, sizeof(xbox360_wired_rumble));
        xdata[3] = lValue;
        xdata[4] = rValue;
        len = sizeof(xbox360_wired_rumble);
        break;
    case XBOXONE:
        memcpy_P(xdata, xboxone_rumble, sizeof(xboxone_rumble));
        xdata[8] = lValue / 2.6f; //Scale is 0 to 100
        xdata[9] = rValue / 2.6f; //Scale is 0 to 100
        len = sizeof(xboxone_rumble);
        break;
    case XBOXOG:
        memcpy_P(xdata, xboxog_rumble, sizeof(xboxog_rumble));
        xdata[2] = lValue;
        xdata[3] = lValue;
        xdata[4] = rValue;
        xdata[5] = rValue;
        len = sizeof(xboxog_rumble);
        break;
    default:
        return hrSUCCESS;
    }

    return pUsb->outTransfer(bAddress, xpad->usbh_outPipe->epAddr, len, xdata);
}

uint8_t XINPUT::SetLed(usbh_xinput_t *xpad, uint8_t quadrant)
{
    xpad->led_actual = xpad->led_requested;
    xpad->timer_out = millis();
    uint16_t len;
    switch (xpad->type)
    {
    case XBOX360_WIRELESS:
        memset(xdata, 0x00, 12);
        memcpy_P(xdata, xbox360w_led, sizeof(xbox360w_led));
        xdata[3] = (quadrant == 0) ? 0x40 : (0x40 | (quadrant + 5));
        len = 12;
        break;
    case XBOX360_WIRED:
        memcpy_P(xdata, xbox360_wired_led, sizeof(xbox360_wired_led));
        xdata[2] = (quadrant == 0) ? 0 : (quadrant + 5);
        len = sizeof(xbox360_wired_led);
        break;
    default:
        return hrSUCCESS;
    }

    return pUsb->outTransfer(bAddress, xpad->usbh_outPipe->epAddr, len, xdata);
}
