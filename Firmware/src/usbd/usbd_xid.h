// Copyright 2020, Ryan Wendland, ogx360
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef XID_h
#define XID_h

#define _USING_XID

#include <stdint.h>
#include <Arduino.h>
#include "PluggableUSB.h"

#define XID_INTERFACECLASS 88
#define XID_INTERFACESUBCLASS 66
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
  uint16_t dButtons;
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
  uint8_t in_dirty;
  uint8_t out_dirty;
} usbd_duke_t;

typedef struct __attribute__((packed))
{
  uint8_t startByte;
  uint8_t bLength;
  uint16_t dButtons;
  uint8_t a;
  uint8_t b;
  uint8_t x;
  uint8_t y;
  uint8_t black;
  uint8_t white;
  uint8_t l;
  uint8_t r;
  int16_t leftStickX;
  int16_t leftStickY;
  int16_t rightStickX;
  int16_t rightStickY;
} xid_steelbattalion_in_t; //FIXME

typedef struct __attribute__((packed))
{
  uint8_t startByte;
  uint8_t bLength;
  uint16_t lValue;
  uint16_t hValue;
} xid_steelbattalion_out_t; //FIXME

class XID_ : public PluggableUSBModule
{
public:
  XID_(void);
  int begin(void);
  int SendReport(const void *data, int len);
  int GetReport(void *data, int len);

protected:
  // Implementation of the PluggableUSBModule
  int getInterface(uint8_t *interfaceCount);
  int getDescriptor(USBSetup &setup);
  bool setup(USBSetup &setup);
  uint8_t getShortName(char *name);

private:
  uint8_t epType[2];
  uint8_t xid_in_data[32];
  uint8_t xid_out_data[32];
};

XID_ &XID();

#endif // XID_h
