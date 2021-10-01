// Copyright 2021, Ryan Wendland, ogx360
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef _XID_h
#define _XID_h

#define _USING_XID

#include <Arduino.h>
#include <PluggableUSB.h>

#define XID_INTERFACECLASS 88
#define XID_INTERFACESUBCLASS 66

#ifndef _USING_HID
#define HID_GET_REPORT 0x01
#define HID_SET_REPORT 0x09
#define HID_REPORT_TYPE_INPUT 1
#define HID_REPORT_TYPE_OUTPUT 2
#endif

#define XID_EP_IN (pluggedEndpoint)
#define XID_EP_OUT (pluggedEndpoint + 1)

#define DUKE_DUP (1 << 0)
#define DUKE_DDOWN (1 << 1)
#define DUKE_DLEFT (1 << 2)
#define DUKE_DRIGHT (1 << 3)
#define DUKE_START (1 << 4)
#define DUKE_BACK (1 << 5)
#define DUKE_LS (1 << 6)
#define DUKE_RS (1 << 7)

//https://github.com/Cxbx-Reloaded/Cxbx-Reloaded/blob/develop/src/core/hle/XAPI/Xapi.h
#define SBC_W0_RIGHTJOYMAINWEAPON 0x0001
#define SBC_W0_RIGHTJOYFIRE 0x0002
#define SBC_W0_RIGHTJOYLOCKON 0x0004
#define SBC_W0_EJECT 0x0008
#define SBC_W0_COCKPITHATCH 0x0010
#define SBC_W0_IGNITION 0x0020
#define SBC_W0_START 0x0040
#define SBC_W0_MULTIMONOPENCLOSE 0x0080
#define SBC_W0_MULTIMONMAPZOOMINOUT 0x0100
#define SBC_W0_MULTIMONMODESELECT 0x0200
#define SBC_W0_MULTIMONSUBMONITOR 0x0400
#define SBC_W0_MAINMONZOOMIN 0x0800
#define SBC_W0_MAINMONZOOMOUT 0x1000
#define SBC_W0_FUNCTIONFSS 0x2000
#define SBC_W0_FUNCTIONMANIPULATOR 0x4000
#define SBC_W0_FUNCTIONLINECOLORCHANGE 0x8000

#define SBC_W1_WASHING 0x0001
#define SBC_W1_EXTINGUISHER 0x0002
#define SBC_W1_CHAFF 0x0004
#define SBC_W1_FUNCTIONTANKDETACH 0x0008
#define SBC_W1_FUNCTIONOVERRIDE 0x0010
#define SBC_W1_FUNCTIONNIGHTSCOPE 0x0020
#define SBC_W1_FUNCTIONF1 0x0040
#define SBC_W1_FUNCTIONF2 0x0080
#define SBC_W1_FUNCTIONF3 0x0100
#define SBC_W1_WEAPONCONMAIN 0x0200
#define SBC_W1_WEAPONCONSUB 0x0400
#define SBC_W1_WEAPONCONMAGAZINE 0x0800
#define SBC_W1_COMM1 0x1000
#define SBC_W1_COMM2 0x2000
#define SBC_W1_COMM3 0x4000
#define SBC_W1_COMM4 0x8000

#define SBC_W2_COMM5 0x0001
#define SBC_W2_LEFTJOYSIGHTCHANGE 0x0002
#define SBC_W2_TOGGLEFILTERCONTROL 0x0004
#define SBC_W2_TOGGLEOXYGENSUPPLY 0x0008
#define SBC_W2_TOGGLEFUELFLOWRATE 0x0010
#define SBC_W2_TOGGLEBUFFREMATERIAL 0x0020
#define SBC_W2_TOGGLEVTLOCATION 0x0040

#define SBC_AXIS_AIMINGX 0x0001
#define SBC_AXIS_AIMINGY 0x0002
#define SBC_AXIS_LEVER   0x0004
#define SBC_AXIS_SIGHTX  0x0008
#define SBC_AXIS_SIGHTY  0x0010
#define SBC_AXIS_LPEDAL  0x0020
#define SBC_AXIS_MPEDAL  0x0040
#define SBC_AXIS_RPEDAL  0x0080
#define SBC_AXIS_TUNER   0x0100
#define SBC_AXIS_GEAR    0x0200

#define SBC_GEAR_R 7
#define SBC_GEAR_N 8
#define SBC_GEAR_1 9
#define SBC_GEAR_2 10
#define SBC_GEAR_3 11
#define SBC_GEAR_4 12
#define SBC_GEAR_5 13

#define SBC_AIMING_MID 32768

static const DeviceDescriptor xid_dev_descriptor PROGMEM =
    D_DEVICE(0x00, 0x00, 0x00, USB_EP_SIZE, USB_VID, USB_PID, 0x0121, 0, 0, 0, 1);

static const uint8_t DUKE_DESC_XID[] PROGMEM = {
    0x10,
    0x42,
    0x00, 0x01,
    0x01,
    0x02,
    0x14,
    0x06,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

static const uint8_t DUKE_CAPABILITIES_IN[] PROGMEM = {
    0x00,
    0x14,
    0xFF,
    0x00,
    0xFF,
    0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF};

static const uint8_t DUKE_CAPABILITIES_OUT[] PROGMEM = {
    0x00,
    0x06,
    0xFF, 0xFF, 0xFF, 0xFF};

static const uint8_t BATTALION_DESC_XID[] PROGMEM = {
    0x10,
    0x42,
    0x00, 0x01,
    0x80,
    0x01,
    0x1A,
    0x16,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

typedef struct __attribute__((packed))
{
    InterfaceDescriptor interface;
    EndpointDescriptor in;
    EndpointDescriptor out;
} XIDDescriptor;

typedef struct __attribute__((packed))
{
    uint8_t startByte;
    uint8_t bLength;
    uint16_t wButtons;
    uint8_t A;
    uint8_t B;
    uint8_t X;
    uint8_t Y;
    uint8_t BLACK;
    uint8_t WHITE;
    uint8_t L;
    uint8_t R;
    int16_t leftStickX;
    int16_t leftStickY;
    int16_t rightStickX;
    int16_t rightStickY;
} usbd_duke_in_t;

typedef struct __attribute__((packed))
{
    uint8_t startByte;
    uint8_t bLength;
    uint16_t lValue;
    uint16_t hValue;
} usbd_duke_out_t;

typedef struct __attribute__((packed))
{
    usbd_duke_in_t in;
    usbd_duke_out_t out;
} usbd_duke_t;

typedef struct __attribute__((packed))
{
    uint8_t startByte;
    uint8_t bLength;
    uint16_t wButtons[3];
    int16_t aimingX;
    int16_t aimingY;
    int16_t rotationLever;
    int16_t sightChangeX;
    int16_t sightChangeY;
    uint16_t leftPedal;
    uint16_t middlePedal;
    uint16_t rightPedal;
    int8_t tunerDial;
    int8_t gearLever;
} usbd_sbattalion_in_t;

typedef struct __attribute__((packed))
{
    uint8_t startByte;
    uint8_t bLength;
    uint8_t CockpitHatch_EmergencyEject;
    uint8_t Start_Ignition;
    uint8_t MapZoomInOut_OpenClose;
    uint8_t SubMonitorModeSelect_ModeSelect;
    uint8_t MainMonitorZoomOut_MainMonitorZoomIn;
    uint8_t Manipulator_ForecastShootingSystem;
    uint8_t Washing_LineColorChange;
    uint8_t Chaff_Extinguisher;
    uint8_t Override_TankDetach;
    uint8_t F1_NightScope;
    uint8_t F3_F2;
    uint8_t SubWeaponControl_MainWeaponControl;
    uint8_t Comm1_MagazineChange;
    uint8_t Comm3_Comm2;
    uint8_t Comm5_Comm4;
    uint8_t GearR_;
    uint8_t Gear1_GearN;
    uint8_t Gear3_Gear2;
    uint8_t Gear5_Gear4;
    uint8_t dummy;
} usbd_sbattalion_out_t;

typedef struct __attribute__((packed))
{
    usbd_sbattalion_in_t in;
    usbd_sbattalion_out_t out;
} usbd_steelbattalion_t;

typedef enum
{
    DISCONNECTED = 0,
    DUKE,
    STEELBATTALION
} xid_type_t;

class XID_ : public PluggableUSBModule
{
public:
    XID_(void);
    int begin(void);
    int sendReport(const void *data, int len);
    int getReport(void *data, int len);
    void setType(xid_type_t type);
    xid_type_t getType(void);

protected:
    int getInterface(uint8_t *interfaceCount);
    int getDescriptor(USBSetup &setup);
    bool setup(USBSetup &setup);

private:
    xid_type_t xid_type;
    uint8_t epType[2];
    uint8_t xid_in_data[32];
    uint8_t xid_out_data[32];
    uint32_t xid_out_expired;
};

XID_ &XID();

#endif // _XID_h
