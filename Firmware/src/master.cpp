/*
This sketch is used for the ogx360 PCB (See https://github.com/Ryzee119/ogx360).
This program incorporates the USB Host Shield Library for the MAX3421 IC See https://github.com/felis/USB_Host_Shield_2.0.
The USB Host Shield Library is an Arduino library, consequently I have imported to necessary Arduino libs into this project.
This program also incorporates the AVR LUFA USB Library, See http://www.fourwalledcubicle.com/LUFA.php for the USB HID support.

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.
This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
GNU General Public License for more details.
You should have received a copy of the GNU General Public License
along with this program. If not, see <https://www.gnu.org/licenses/>.

In settings.h you can configure the following options:
1. Compile for MASTER of SLAVE (comment out #define MASTER) (Host is default)
2. Enable or disable Steel Battalion Controller Support via Wireless Xbox 360 Chatpad (Enabled by Default)
3. Enable or disable Xbox 360 Wired Support (Enabled by default)
4. Enable or disable Xbox One Wired Support (Disabled by default)
*/

#include <Arduino.h>
#include <Wire.h>
#include <EEPROM.h>
#include <UHS2/Usb.h>
#include <UHS2/usbhub.h>

#include "usbd/usbd_xid.h"
#include "usbh/usbh_xinput.h"
#include "config.h"

USB UsbHost;
USBHub Hub(&UsbHost);
XINPUT xinput1(&UsbHost);
XINPUT xinput2(&UsbHost);
XINPUT xinput3(&UsbHost);
XINPUT xinput4(&UsbHost);

void master_init(usbd_duke_t * usbd_head, uint8_t max_controllers)
{
    pinMode(USB_HOST_RESET_PIN, OUTPUT);
    digitalWrite(USB_HOST_RESET_PIN, LOW);

    memset(usbd_head, 0x00, sizeof(usbd_duke_t) * max_controllers);
    for (uint8_t i = 0; i < max_controllers; i++)
    {
        usbd_head[i].in.bLength = sizeof(usbd_duke_in_t);
        usbd_head[i].out.bLength = sizeof(usbd_duke_out_t);
    }

    //Init Usb Host Controller
    digitalWrite(USB_HOST_RESET_PIN, LOW);
    delay(20);
    digitalWrite(USB_HOST_RESET_PIN, HIGH);
    delay(20);
    while (UsbHost.Init() == -1)
    {
        digitalWrite(ARDUINO_LED_PIN, !digitalRead(ARDUINO_LED_PIN));
        delay(500);
    }

    //Ping slave devices if present. This will cause them to blink
    for (uint8_t i = 1; i < max_controllers; i++)
    {
        static const char ping = 0xAA;
        Wire.beginTransmission(i);
        Wire.write(&ping, 1);
        Wire.endTransmission(true);
        delay(100);
    }
}

void PrintHex8(uint8_t *data, uint8_t length, bool lf) // prints 8-bit data in hex with leading zeroes
{
    char tmp[16];
    for (int i = 0; i < length; i++)
    {
        sprintf(tmp, "0x%.2X", data[i]);
        Serial1.print(tmp);
        Serial1.print(" ");
    }
    if (lf)
        Serial1.print("\n");
    else
        Serial1.print(" ");
}

void master_task(usbd_duke_t *usbd_head, uint8_t max_controllers)
{
    UsbHost.busprobe();
    UsbHost.Task();

    usbd_duke_t *usbd_dev = usbd_head;
    usbh_xinput_t *usbh_xhead = usbh_xinput_get_device_list();
    for (int i = 0; i < max_controllers; i++)
    {
        usbh_xinput_t *usbh_xin = &usbh_xhead[i];

        if (usbh_xin->bAddress == 0)
        {   
            if (i == 0)
            {
                //Disconnect OG Xbox size USB device
                UDCON |= (1 << DETACH);
            }
            else
            {
                //No controller connected, let the slave device know
                static const uint8_t disablePacket = 0xF0;
                Wire.beginTransmission(i);
                Wire.write((char *)(&disablePacket), 1);
                Wire.endTransmission(true);
            }
            continue;
        }

        //Connect OG Xbox size USB device if not connected
        if (i==0)
            UDCON &= ~(1 << DETACH);

        xinput_padstate_t *usbh_xstate = &usbh_xin->pad_state;
        memset(&usbd_dev[i].in.dButtons, 0x00, 10);

        if (usbh_xstate->wButtons & (1 << 0)) usbd_dev[i].in.dButtons |= DUKE_DUP;
        if (usbh_xstate->wButtons & (1 << 1)) usbd_dev[i].in.dButtons |= DUKE_DDOWN;
        if (usbh_xstate->wButtons & (1 << 2)) usbd_dev[i].in.dButtons |= DUKE_DLEFT;
        if (usbh_xstate->wButtons & (1 << 3)) usbd_dev[i].in.dButtons |= DUKE_DRIGHT;
        if (usbh_xstate->wButtons & (1 << 4)) usbd_dev[i].in.dButtons |= DUKE_START;
        if (usbh_xstate->wButtons & (1 << 5)) usbd_dev[i].in.dButtons |= DUKE_BACK;
        if (usbh_xstate->wButtons & (1 << 6)) usbd_dev[i].in.dButtons |= DUKE_LS;
        if (usbh_xstate->wButtons & (1 << 7)) usbd_dev[i].in.dButtons |= DUKE_RS;

        //Analog buttons are converted to digital
        if (usbh_xstate->wButtons & (1 << 8))  usbd_dev[i].in.WHITE = 0xFF;
        if (usbh_xstate->wButtons & (1 << 9))  usbd_dev[i].in.BLACK = 0xFF;
        if (usbh_xstate->wButtons & (1 << 12)) usbd_dev[i].in.A = 0xFF;
        if (usbh_xstate->wButtons & (1 << 13)) usbd_dev[i].in.B = 0xFF;
        if (usbh_xstate->wButtons & (1 << 14)) usbd_dev[i].in.X = 0xFF;
        if (usbh_xstate->wButtons & (1 << 15)) usbd_dev[i].in.Y = 0xFF;

        //Analog Sticks
        usbd_dev[i].in.leftStickX  = usbh_xstate->sThumbLX;
        usbd_dev[i].in.leftStickY  = usbh_xstate->sThumbLY;
        usbd_dev[i].in.rightStickX = usbh_xstate->sThumbRX;
        usbd_dev[i].in.rightStickY = usbh_xstate->sThumbRY;
        usbd_dev[i].in.L           = usbh_xstate->bLeftTrigger;
        usbd_dev[i].in.R           = usbh_xstate->bRightTrigger;

        if (i != 0)
        {
            //Send controller data to slaves
            Wire.beginTransmission(i);
            Wire.write((char *)&usbd_dev[i].in, sizeof(usbd_duke_in_t));
            Wire.endTransmission(true);

            //Get rumble data from slaves
            if (Wire.requestFrom(i, 2) == 2)
            {
                uint8_t l, r;
                l = Wire.read();
                r = Wire.read();
                if (l != -1) usbd_dev[i].out.lValue = l << 8;
                if (r != -1) usbd_dev[i].out.hValue = r << 8;
            }
            else
            {
                Wire.flush();
                usbd_dev[i].out.lValue = 0;
                usbd_dev[i].out.hValue = 0;
            }
        }

        usbh_xin->lValue_requested = usbd_dev[i].out.lValue >> 8;
        usbh_xin->rValue_requested = usbd_dev[i].out.hValue >> 8;
    }
}
