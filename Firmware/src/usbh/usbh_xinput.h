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
 */

#ifndef _xboxusb_h_
#define _xboxusb_h_

#include "UHS2/Usb.h"

/* Names we give to the 3 Xbox360 pipes */
#define XBOX_CONTROL_PIPE 0
#define XBOX_INPUT_PIPE 1
#define XBOX_OUTPUT_PIPE 2

#define EP_MAXPKTSIZE 33
#define XBOX_MAX_ENDPOINTS 5

typedef enum
{
    XINPUT_UNKNOWN = 0,
    XBOXONE,
    XBOX360_WIRELESS,
    XBOX360_WIRED
} xidtype_t;

/** This class implements support for a Xbox wired controller via USB. */
class XINPUT : public USBDeviceConfig
{
public:
    XINPUT(USB *pUsb);

    uint8_t ConfigureDevice(uint8_t parent, uint8_t port, bool lowspeed);
    uint8_t Init(uint8_t parent, uint8_t port, bool lowspeed);
    uint8_t Release();
    uint8_t Poll();
    virtual uint8_t GetAddress()
    {
        return bAddress;
    };
    virtual bool isReady()
    {
        return bIsReady;
    };

protected:
    USB *pUsb;
    uint8_t bAddress;
    EpInfo epInfo[XBOX_MAX_ENDPOINTS];

private:
    bool bIsReady;
    xidtype_t xid_type;
};
#endif
