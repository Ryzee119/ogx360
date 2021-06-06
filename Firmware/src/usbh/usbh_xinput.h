// Copyright 2020, Ryan Wendland, ogx360
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef _XINPUT_H_
#define _XINPUT_H_

#include "UHS2/Usb.h"

#define XBOX_CONTROL_PIPE 0
#define XBOX_INPUT_PIPE 1
#define XBOX_OUTPUT_PIPE 2

#define EP_MAXPKTSIZE 32
#define XBOX_MAX_ENDPOINTS 9

#ifndef XINPUT_MAXGAMEPADS
#define XINPUT_MAXGAMEPADS 4
#endif

//https://docs.microsoft.com/en-us/windows/win32/api/xinput/ns-xinput-xinput_gamepad
#define XINPUT_GAMEPAD_DPAD_UP 0x0001
#define XINPUT_GAMEPAD_DPAD_DOWN 0x0002
#define XINPUT_GAMEPAD_DPAD_LEFT 0x0004
#define XINPUT_GAMEPAD_DPAD_RIGHT 0x0008
#define XINPUT_GAMEPAD_START 0x0010
#define XINPUT_GAMEPAD_BACK 0x0020
#define XINPUT_GAMEPAD_LEFT_THUMB 0x0040
#define XINPUT_GAMEPAD_RIGHT_THUMB 0x0080
#define XINPUT_GAMEPAD_LEFT_SHOULDER 0x0100
#define XINPUT_GAMEPAD_RIGHT_SHOULDER 0x0200
#define XINPUT_GAMEPAD_A 0x1000
#define XINPUT_GAMEPAD_B 0x2000
#define XINPUT_GAMEPAD_X 0x4000
#define XINPUT_GAMEPAD_Y 0x8000

typedef struct
{
    uint16_t wButtons;
    uint8_t bLeftTrigger;
    uint8_t bRightTrigger;
    int16_t sThumbLX;
    int16_t sThumbLY;
    int16_t sThumbRX;
    int16_t sThumbRY;
} xinput_padstate_t;

typedef enum
{
    XINPUT_UNKNOWN = 0,
    XBOXONE,
    XBOX360_WIRELESS,
    XBOX360_WIRED
} xinput_type_t;

typedef struct usbh_xinput_t
{
    //usbh backend handles
    uint8_t bAddress;
    EpInfo *usbh_inPipe;  //Pipe specific to this pad
    EpInfo *usbh_outPipe; //Pipe specific to this pad
    //xinput controller state
    xinput_padstate_t pad_state; //Current pad button/stick state
    uint8_t lValue_requested;    //Requested left rumble value
    uint8_t rValue_requested;    //Requested right rumble value
    uint8_t lValue_actual;
    uint8_t rValue_actual;
    uint8_t led_requested;       //Requested led quadrant, 1-4 or 0 for off
    uint8_t led_actual;
    //Chatpad specific components
    uint8_t chatpad_initialised;
    uint8_t chatpad_state[3];
    uint8_t chatpad_led_requested;
    uint8_t chatpad_led_actual;
    uint32_t timer_periodic;
    uint32_t timer_out;
} usbh_xinput_t;

usbh_xinput_t *usbh_xinput_get_device_list(void);

//Rumble commands
static const uint8_t xbox360_wireless_rumble[] PROGMEM = {0x00, 0x01, 0x0F, 0xC0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
static const uint8_t xbox360_wired_rumble[] PROGMEM = {0x00, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
static const uint8_t xbox_one_rumble[] PROGMEM = {0x09, 0x00, 0x00, 0x09, 0x00, 0x0f, 0x00, 0x00, 0x00, 0x00, 0xFF, 0x00, 0xEB};

//Led commands
static const uint8_t xbox360_wireless_led[] PROGMEM = {0x00, 0x00, 0x08, 0x40, 0x00, 0x00, 0x00, 0x00};
static const uint8_t xbox360_wired_led[] PROGMEM = {0x01, 0x03, 0x00};

//Init commands
static const uint8_t xboxone_start_input[] PROGMEM = {0x05, 0x20, 0x00, 0x01, 0x00};
static const uint8_t xboxone_s_init[] PROGMEM = {0x05, 0x20, 0x00, 0x0f, 0x06};
static const uint8_t xboxone_pdp_init1[] PROGMEM = {0x0a, 0x20, 0x00, 0x03, 0x00, 0x01, 0x14};
static const uint8_t xboxone_pdp_init2[] PROGMEM = {0x06, 0x30};
static const uint8_t xboxone_pdp_init3[] PROGMEM = {0x06, 0x20, 0x00, 0x02, 0x01, 0x00};

//Other Commands
static const uint8_t xbox360w_inquire_present[] PROGMEM = {0x08, 0x00, 0x0F, 0xC0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
static const uint8_t xbox360w_controller_info[] PROGMEM = {0x00, 0x00, 0x00, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
static const uint8_t xbox360w_chatpad_init[] PROGMEM = {0x00, 0x00, 0x0C, 0x1B};
static const uint8_t xbox360w_chatpad_keepalive1[] PROGMEM = {0x00, 0x00, 0x0C, 0x1F};
static const uint8_t xbox360w_chatpad_keepalive2[] PROGMEM = {0x00, 0x00, 0x0C, 0x1E};
static const uint8_t xbox360w_chatpad_led_ctrl[] PROGMEM = {0x00, 0x00, 0x0C, 0x00};
#define CHATPAD_CAPSLOCK 0x20
#define CHATPAD_GREEN 0x08
#define CHATPAD_ORANGE 0x10
#define CHATPAD_MESSENGER 0x01

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
    uint16_t PID,VID;
#ifdef ENABLE_XINPUT_STRINGS
    uint8_t iProduct, iManuf, iSerial;
#endif
    xinput_type_t xinput_type;
    usbh_xinput_t *alloc_xinput_device(uint8_t bAddress, EpInfo *in, EpInfo *out);
    uint8_t free_xinput_device(usbh_xinput_t *xinput_dev);
    int8_t get_xinput_device_index(usbh_xinput_t *xinput);
    bool ParseInputData(usbh_xinput_t **xpad, EpInfo *ep_in);
    bool GetRumbleCommand(usbh_xinput_t *xpad, uint8_t *tdata, uint8_t *len, uint8_t lValue, uint8_t rValue);
    bool GetLedCommand(usbh_xinput_t *xpad, uint8_t *tdata, uint8_t *len, uint8_t quadrant);
};
#endif
