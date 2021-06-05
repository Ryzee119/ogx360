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

xid_gamepad xboxog_duke[MAX_GAMEPADS];

xinput_pad_t xinput_pad[MAX_GAMEPADS];

void master_init()
{
    pinMode(USB_HOST_RESET_PIN, OUTPUT);
    digitalWrite(USB_HOST_RESET_PIN, LOW);

    memset(xboxog_duke, 0x00, sizeof(xid_gamepad) * MAX_GAMEPADS);
    for (uint8_t i = 0; i < MAX_GAMEPADS; i++)
    {
        xboxog_duke[i].in.bLength = sizeof(xid_gamepad_in);
        xboxog_duke[i].out.bLength = sizeof(xid_gamepad_out);
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
    for (uint8_t i = 1; i < MAX_GAMEPADS; i++)
    {
        static const char ping = 0xAA;
        Wire.beginTransmission(i);
        Wire.write(&ping, 1);
        Wire.endTransmission(true);
        delay(100);
    }
}

void PrintHex8(uint8_t *data, uint8_t length) // prints 8-bit data in hex with leading zeroes
{
    char tmp[16];
    for (int i = 0; i < length; i++)
    {
        sprintf(tmp, "0x%.2X", data[i]);
        Serial1.print(tmp);
        Serial1.print(" ");
    }
    Serial1.print("\n");
}

void master_task()
{
    static uint32_t tick = 0;

    UsbHost.busprobe();
    UsbHost.Task();
    xinput_pad_t *aa = usbh_xinput_get_device_list();
    uint32_t index = 0;
    static uint32_t j = 0;
    static uint8_t r = 0x55;
    if (millis() - tick > 10)
    {
        while (aa != NULL)
        {
            if (aa->bAddress)
            {
                //Serial1.print(String(index) + ": ");
                //PrintHex8((uint8_t *)&(aa->pad_state), 12);
                //aa->led_requested = (j++ % 4) + 1;
                aa->rValue_requested = r;
                aa->lValue_requested = r;
                r++;
                index++;
            }
            aa = aa->next;
        }
        tick = millis();
    }
}