#include "usbh_xinput.h"

#define ENABLE_USBH_XINPUT_DEBUG
#ifdef ENABLE_USBH_XINPUT_DEBUG
#define USBH_XINPUT_DEBUG(a) Serial1.print(a)
#else
#define USBH_XINPUT_DEBUG(...)
#endif

XINPUT::XINPUT(USB *p) : pUsb(p),
                         bAddress(0),
                         bIsReady(false),
                         xid_type(XINPUT_UNKNOWN)
{
    if (pUsb)
    {
        pUsb->RegisterDeviceClass(this);
    }
}

//FIXME: Hardcoded conf descriptor length. What if its alots larger? Prob dont have enough RAM for this :(
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

    USB_INTERFACE_DESCRIPTOR *uid = reinterpret_cast<USB_INTERFACE_DESCRIPTOR *>(conf_desc_buff +
                                                                                 sizeof(USB_CONFIGURATION_DESCRIPTOR));

    rcode = pUsb->getConfDescr(0, 0, conf_desc_len, 0, (uint8_t *)conf_desc_buff);
    if (rcode)
    {
        USBH_XINPUT_DEBUG(F("XINPUT: getConfDescr error\n"));
        return rcode;
    }

    //Check the device descriptor here. Interface class is checked later.
    if (udd.bDeviceClass != 0xFF && udd.bDeviceClass != 0x00)
    {
        USBH_XINPUT_DEBUG(F("XINPUT: USB_DEV_CONFIG_ERROR_DEVICE_NOT_SUPPORTED. Failed DeviceClass"));
        return USB_DEV_CONFIG_ERROR_DEVICE_NOT_SUPPORTED;
    }

    xid_type = XINPUT_UNKNOWN;
    if (uid->bNumEndpoints < 2)
        xid_type = XINPUT_UNKNOWN;
    else if (uid->bInterfaceSubClass == 0x5D && //Xbox360 wireless bInterfaceSubClass
             uid->bInterfaceProtocol == 0x81)   //Xbox360 wireless bInterfaceProtocol
        xid_type = XBOX360_WIRELESS;
    else if (uid->bInterfaceSubClass == 0x5D && //Xbox360 wired bInterfaceSubClass
             uid->bInterfaceProtocol == 0x01)   //Xbox360 wired bInterfaceProtocol
        xid_type = XBOX360_WIRED;
    else if (uid->bInterfaceSubClass == 0x47 && //Xbone and SX bInterfaceSubClass
             uid->bInterfaceProtocol == 0xD0)   //Xbone and SX bInterfaceProtocol
        xid_type = XBOXONE;

    if (xid_type == XINPUT_UNKNOWN)
    {
        USBH_XINPUT_DEBUG(F("XINPUT: XINPUT_UNKNOWN\n"));
        return USB_DEV_CONFIG_ERROR_DEVICE_NOT_SUPPORTED;
    }

    USBH_XINPUT_DEBUG(F("Xid type: "));
    USBH_XINPUT_DEBUG(xid_type);
    USBH_XINPUT_DEBUG("\n");

    epInfo[0].epAddr = 0x00;
    epInfo[0].epAttribs = USB_TRANSFER_TYPE_CONTROL;
    epInfo[0].maxPktSize = udd.bMaxPacketSize0;
    epInfo[0].bmNakPower = USB_NAK_MAX_POWER;

    //So far so good, lets issue a reset again, then finish everything off
    return USB_ERROR_CONFIG_REQUIRES_ADDITIONAL_RESET;
};

uint8_t XINPUT::Init(uint8_t parent __attribute__((unused)), uint8_t port __attribute__((unused)), bool lowspeed)
{
    uint8_t rcode;

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

    delay(100); //Give time for address change

    UsbDevice *p = addrPool.GetUsbDevicePtr(bAddress);
    if (!p)
    {
        Release();
        USBH_XINPUT_DEBUG(F("XINPUT: GetUsbDevicePtr error\n"));
        return USB_ERROR_ADDRESS_NOT_FOUND_IN_POOL;
    }

    p->lowspeed = lowspeed;

    for (uint8_t i = 1; i < XBOX_MAX_ENDPOINTS; i++)
    {
        //Force known ep addresses for wireless receiver (1,3,4,5)
        epInfo[i].epAddr = xid_type == XBOX360_WIRELESS ? ((i * 2) - 1) : 0;
        epInfo[i].epAttribs = USB_TRANSFER_TYPE_INTERRUPT;
        epInfo[i].bmNakPower = USB_NAK_NOWAIT; // Only poll once for interrupt endpoints
        epInfo[i].maxPktSize = EP_MAXPKTSIZE;
        epInfo[i].bmSndToggle = 0;
        epInfo[i].bmRcvToggle = 0;
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
                epInfo[pipe].epAddr = uepd->bEndpointAddress;
                epInfo[pipe].epAttribs = uepd->bmAttributes;
            }
        }
        cd_pos += cd_len;
    }

    rcode = pUsb->setEpInfoEntry(bAddress, xid_type == XBOX360_WIRELESS ? 5 : 3, epInfo);
    if (rcode)
    {
        Release();
        USBH_XINPUT_DEBUG(F("XINPUT: setEpInfoEntry error\n"));
        return rcode;
    }

    rcode = pUsb->setConf(bAddress, epInfo[XBOX_CONTROL_PIPE].epAddr, ucd->bConfigurationValue);
    if (rcode)
    {
        Release();
        USBH_XINPUT_DEBUG(F("XINPUT: setConf error\n"));
        return rcode;
    }

    bIsReady = true;
    USBH_XINPUT_DEBUG(F("XINPUT ENUMERATED OK!\n"));
    return 0;
}

/* Performs a cleanup after failed Init() attempt */
uint8_t XINPUT::Release()
{
    pUsb->GetAddressPool().FreeAddress(bAddress);
    bAddress = 0;
    bIsReady = false;
    return 0;
}

uint8_t XINPUT::Poll()
{
    if (!bIsReady)
        return 0;
    return 1;
}