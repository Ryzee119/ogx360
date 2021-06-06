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

void master_init(usbd_duke_t *usbd_head, uint8_t len);
void master_task(usbd_duke_t *usbd_head, uint8_t len);

static inline void USB_Detach(void)
{
    UDCON |= (1 << DETACH);
}

static inline void USB_Attach(void)
{
    UDCON &= ~(1 << DETACH);
}

uint8_t playerID;
XID_ usbd_xid;
usbd_duke_t usbd_duke[MAX_GAMEPADS];

void setup()
{
    //Init IO
    pinMode(USB_HOST_RESET_PIN, OUTPUT);
    pinMode(ARDUINO_LED_PIN, OUTPUT);
    pinMode(PLAYER_ID1_PIN, INPUT_PULLUP);
    pinMode(PLAYER_ID2_PIN, INPUT_PULLUP);
    digitalWrite(USB_HOST_RESET_PIN, LOW);
    digitalWrite(ARDUINO_LED_PIN, HIGH);

    //Initialise the Serial Port
    Serial1.begin(115200);
    Serial1.print("hello!\n");

    //Determine what player this board is. Used for the slave devices mainly.
    //There is 2 ID pins on the PCB which are read in.
    //00 = Player 1
    //01 = Player 2
    //10 = Player 3
    //11 = Player 4
    playerID = digitalRead(PLAYER_ID1_PIN) << 1 | digitalRead(PLAYER_ID2_PIN);

    master_init(&usbd_duke[0], MAX_GAMEPADS);

    //Init I2C Master
    Wire.begin();
    Wire.setClock(400000);

    //Ping slave devices if present
    //This will cause them to blink
    for (uint8_t i = 1; i < MAX_CONTROLLERS; i++)
    {
        static const char ping = 0xAA;
        Wire.beginTransmission(i);
        Wire.write(&ping, 1);
        Wire.endTransmission(true);
        delay(100);
    }
}

void loop()
{
    master_task(&usbd_duke[0], MAX_GAMEPADS);

    static uint32_t poll_timer = 0;
    if (millis() - poll_timer > 4)
    {
        usbd_xid.SendReport(&usbd_duke[0].in, sizeof(usbd_duke_in_t));
        usbd_xid.GetReport(&usbd_duke[0].out, sizeof(usbd_duke_out_t));
        poll_timer = millis();
    }
}