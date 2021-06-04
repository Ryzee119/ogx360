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

#define DUP (1 << 0)
#define DDOWN (1 << 1)
#define DLEFT (1 << 2)
#define DRIGHT (1 << 3)
#define START_BTN (1 << 4)
#define BACK_BTN (1 << 5)
#define LS_BTN (1 << 6)
#define RS_BTN (1 << 7)


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
} xid_gamepad_in;

typedef struct __attribute__((packed))
{
  uint8_t startByte;
  uint8_t bLength;
  uint16_t lValue;
  uint16_t hValue;
} xid_gamepad_out;

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
} xid_steelbattalion_in; //FIXME

typedef struct __attribute__((packed))
{
  uint8_t startByte;
  uint8_t bLength;
  uint16_t lValue;
  uint16_t hValue;
} xid_steelbattalion_out; //FIXME

class XID_ : public PluggableUSBModule
{
public:
  XID_(void);
  int begin(void);
  int SendReport(uint8_t id, const void *data, int len);

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
