// Copyright 2020, Ryan Wendland, ogx360
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef _XINPUT_H_
#define _XINPUT_H_

#include "UHS2/Usb.h"

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
