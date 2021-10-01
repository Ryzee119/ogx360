// Copyright 2021, Ryan Wendland, ogx360
// SPDX-License-Identifier: GPL-3.0-or-later

#include "usbd_xid.h"

//#define ENABLE_USBD_XID_DEBUG
#ifdef ENABLE_USBD_XID_DEBUG
#define USBD_XID_DEBUG(a) Serial1.print(F(a))
#else
#define USBD_XID_DEBUG(...)
#endif

XID_ &XID()
{
    static XID_ obj;
    return obj;
}

int XID_::getInterface(uint8_t *interfaceCount)
{
    *interfaceCount += 1;

    XIDDescriptor xid_interface = {
        D_INTERFACE(pluggedInterface, 2, XID_INTERFACECLASS, XID_INTERFACESUBCLASS, 0),
        D_ENDPOINT(USB_ENDPOINT_IN(XID_EP_IN), USB_ENDPOINT_TYPE_INTERRUPT, USB_EP_SIZE, 0x04),
        D_ENDPOINT(USB_ENDPOINT_OUT(XID_EP_OUT), USB_ENDPOINT_TYPE_INTERRUPT, USB_EP_SIZE, 0x04)};

    return USB_SendControl(0, &xid_interface, sizeof(xid_interface));
}

int XID_::getDescriptor(USBSetup &setup)
{
    //Device descriptor for duke, seems to work fine for Steel Battalion. Keep constant.
    USB_SendControl(TRANSFER_PGM, &xid_dev_descriptor, sizeof(xid_dev_descriptor));
    return sizeof(xid_dev_descriptor);
}

int XID_::sendReport(const void *data, int len)
{
    int capped_len = min((unsigned int)len, sizeof(xid_in_data));
    if (memcmp(xid_in_data, data, capped_len) != 0)
    {
        //Update local copy, then send
        if (USB_Send(XID_EP_IN | TRANSFER_RELEASE, data, capped_len) == len)
            memcpy(xid_in_data, data, capped_len);
    }
    return len;
}

int XID_::getReport(void *data, int len)
{
    int capped_len = min((uint32_t)len, sizeof(xid_out_data));
    uint8_t r[capped_len] = {0};
    if (USB_Recv(XID_EP_OUT | TRANSFER_RELEASE, r, capped_len) == capped_len)
    {
        USBD_XID_DEBUG("USBD XID: GOT HID REPORT OUT FROM ENDPOINT\n");
        memcpy(xid_out_data, r, capped_len);
        memcpy(data, r, capped_len);
        xid_out_expired = millis();
        return capped_len;
    }
    //No new data on interrupt pipe, if its been a while since last update,
    //Treat it as expired. Prevents rumble locking on old values.
    if(millis() - xid_out_expired > 500)
    {
        memset(data, 0x00, capped_len);
        return 0;
    }
    //No new data on interrupt pipe, return previous data
    memcpy(data, xid_out_data, capped_len);
    return xid_out_data[1];
}

bool XID_::setup(USBSetup &setup)
{
    if (pluggedInterface != setup.wIndex)
    {
        return false;
    }

    uint8_t request = setup.bRequest;
    uint8_t requestType = setup.bmRequestType;
    uint16_t wValue = (setup.wValueH << 8) | (setup.wValueL & 0xFF);

    if (requestType == (REQUEST_DEVICETOHOST | REQUEST_VENDOR | REQUEST_INTERFACE))
    {
        if (request == 0x06 && wValue == 0x4200)
        {
            USBD_XID_DEBUG("USBD XID: SENDING XID DESCRIPTOR\n");
            if (xid_type == DUKE)
            {
                USB_SendControl(TRANSFER_PGM, DUKE_DESC_XID, sizeof(DUKE_DESC_XID));
            }
            else if (xid_type == STEELBATTALION)
            {
                USB_SendControl(TRANSFER_PGM, BATTALION_DESC_XID, sizeof(BATTALION_DESC_XID));
            }
            return true;
        }
        if (request == 0x01 && wValue == 0x0100)
        {
            USBD_XID_DEBUG("USBD XID: SENDING XID CAPABILITIES IN\n");
            USB_SendControl(TRANSFER_PGM, DUKE_CAPABILITIES_IN, sizeof(DUKE_CAPABILITIES_IN));
            return true;
        }
        if (request == 0x01 && wValue == 0x0200)
        {
            USBD_XID_DEBUG("USBD XID: SENDING XID CAPABILITIES OUT\n");
            USB_SendControl(TRANSFER_PGM, DUKE_CAPABILITIES_OUT, sizeof(DUKE_CAPABILITIES_OUT));
            return true;
        }
    }

    if (requestType == (REQUEST_DEVICETOHOST | REQUEST_CLASS | REQUEST_INTERFACE))
    {
        if (request == HID_GET_REPORT && setup.wValueH == HID_REPORT_TYPE_INPUT)
        {
            USBD_XID_DEBUG("USBD XID: SENDING HID REPORT IN\n");
            USB_SendControl(0, xid_in_data, sizeof(xid_in_data));
            return true;
        }
    }

    if (requestType == (REQUEST_HOSTTODEVICE | REQUEST_CLASS | REQUEST_INTERFACE))
    {
        if (request == HID_SET_REPORT && setup.wValueH == HID_REPORT_TYPE_OUTPUT)
        {
            USBD_XID_DEBUG("USBD XID: GETTING HID REPORT OUT\n");
            uint16_t length = min(sizeof(xid_out_data), setup.wLength);
            USB_RecvControl(xid_out_data, length);
            xid_out_expired = millis();
            return true;
        }
    }

    USBD_XID_DEBUG("USBD XID: STALL\n");
    Serial1.println(requestType, HEX);
    Serial1.println(request, HEX);
    Serial1.println(wValue, HEX);
    return false;
}

void XID_::setType(xid_type_t type)
{
    if (xid_type == type)
    {
        return;
    }

    xid_type = type;
    UDCON |= (1 << DETACH);
    delay(10);
    if (xid_type != DISCONNECTED)
    {
        UDCON &= ~(1 << DETACH);
    } 
    return;
}

xid_type_t XID_::getType(void)
{
    return xid_type;
}

XID_::XID_(void) : PluggableUSBModule(2, 1, epType)
{
    epType[0] = EP_TYPE_INTERRUPT_IN;
    epType[1] = EP_TYPE_INTERRUPT_OUT;
    memset(xid_out_data, 0x00, sizeof(xid_out_data));
    memset(xid_in_data, 0x00, sizeof(xid_in_data));
    xid_type = DUKE;
    PluggableUSB().plug(this);
}

int XID_::begin(void)
{
    return 0;
}
