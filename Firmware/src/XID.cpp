/*
   Copyright (c) 2015, Arduino LLC
   Original code (pre-library): Copyright (c) 2011, Peter Barrett

   Permission to use, copy, modify, and/or distribute this software for
   any purpose with or without fee is hereby granted, provided that the
   above copyright notice and this permission notice appear in all copies.

   THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL
   WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED
   WARRANTIES OF MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR
   BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES
   OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS,
   WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION,
   ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS
   SOFTWARE.
 */

#include "HID.h"
#include "XID.h"

#if defined(USBCON)

#define ENABLE_USBH_XID_DEBUG
#ifdef ENABLE_USBH_XID_DEBUG
#define USBH_XID_DEBUG(a) Serial1.print(F(a))
#else
#define USBH_XID_DEBUG(...)
#endif

XID_ &XID()
{
    static XID_ obj;
    return obj;
}

int XID_::getInterface(uint8_t *interfaceCount)
{
    *interfaceCount += 1; // uses 1

    XIDDescriptor xid_interface = {
        D_INTERFACE(pluggedInterface, 2, XID_INTERFACECLASS, XID_INTERFACESUBCLASS, 0),
        D_ENDPOINT(USB_ENDPOINT_IN(XID_EP_IN), USB_ENDPOINT_TYPE_INTERRUPT, USB_EP_SIZE, 0x04),
        D_ENDPOINT(USB_ENDPOINT_OUT(XID_EP_OUT), USB_ENDPOINT_TYPE_INTERRUPT, USB_EP_SIZE, 0x04)};
    return USB_SendControl(0, &xid_interface, sizeof(xid_interface));
}

int XID_::getDescriptor(USBSetup &setup)
{
    USB_SendControl(TRANSFER_PGM, &xid_dev_descriptor, sizeof(xid_dev_descriptor));
    return sizeof(xid_dev_descriptor);
}

uint8_t XID_::getShortName(char *name)
{
    name[0] = 'D';
    name[1] = 'U';
    name[2] = 'K';
    name[3] = 'E';
    return 4;
}

int XID_::SendReport(uint8_t id, const void *data, int len)
{
    int capped_len = min((unsigned int)len, sizeof(xid_in_data));
    memcpy(xid_in_data, data, capped_len);
    USB_Send(XID_EP_IN | TRANSFER_RELEASE, xid_in_data, capped_len);
    return len;
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
            USBH_XID_DEBUG("Sending XID Descriptor\n");
            USB_SendControl(0, DUKE_DESC_XID, sizeof(DUKE_DESC_XID));
            //USB_SendControl(0, BATTALION_DESC_XID, sizeof(BATTALION_DESC_XID));
            return true;
        }
        if (request == 0x01 && wValue == 0x0100)
        {
            USBH_XID_DEBUG("Sending XID Capabilities IN\n");
            USB_SendControl(0, DUKE_CAPABILITIES_IN, sizeof(DUKE_CAPABILITIES_IN));
            return true;
        }
        if (request == 0x01 && wValue == 0x0200)
        {
            USBH_XID_DEBUG("Sending XID Capabilities OUT\n");
            USB_SendControl(0, DUKE_CAPABILITIES_OUT, sizeof(DUKE_CAPABILITIES_OUT));
            return true;
        }
    }

    if (requestType == (REQUEST_DEVICETOHOST | REQUEST_CLASS | REQUEST_INTERFACE))
    {
        if (request == 0x01 && wValue == 0x0100)
        {
            USBH_XID_DEBUG("Sending HID Report IN\n");
            USB_SendControl(0, xid_in_data, sizeof(xid_in_data));
            return true;
        }
        if (request == 0x09 && wValue == 0x0200)
        {
            USBH_XID_DEBUG("Getting HID Report Out\n");
            uint16_t length = min(sizeof(xid_out_data), setup.wLength);
            USB_RecvControl(xid_out_data, length);
            return true;
        }
    }

    USBH_XID_DEBUG("STALL\n");
    return false;
}

XID_::XID_(void) : PluggableUSBModule(2, 1, epType)
{
    epType[0] = EP_TYPE_INTERRUPT_IN;
    epType[1] = EP_TYPE_INTERRUPT_OUT;
    PluggableUSB().plug(this);
}

int XID_::begin(void)
{
    return 0;
}

#endif /* if defined(USBCON) */
