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
#include "main.h"

void master_init();
void master_task();

uint8_t playerID;
XID_ usbd_xid;
usbd_controller_t usbd_c[MAX_GAMEPADS];

void setup()
{
    //Init IO
    pinMode(ARDUINO_LED_PIN, OUTPUT);
    pinMode(PLAYER_ID1_PIN, INPUT_PULLUP);
    pinMode(PLAYER_ID2_PIN, INPUT_PULLUP);
    digitalWrite(ARDUINO_LED_PIN, HIGH);

    memset(usbd_c, 0x00, sizeof(usbd_controller_t) * MAX_GAMEPADS);
    for (uint8_t i = 0; i < MAX_GAMEPADS; i++)
    {
        usbd_c[i].type = DUKE;
        usbd_c[i].duke.in.bLength = sizeof(usbd_duke_in_t);
        usbd_c[i].duke.out.bLength = sizeof(usbd_duke_out_t);

        usbd_c[i].sb.in.bLength = sizeof(usbd_sbattalion_in_t);
        usbd_c[i].sb.out.bLength = sizeof(usbd_sbattalion_out_t);
    }

    //Determine what player this board is. Used for the slave devices mainly.
    //There is 2 ID pins on the PCB which are read in.
    //00 = Player 1
    //01 = Player 2
    //10 = Player 3
    //11 = Player 4
    playerID = digitalRead(PLAYER_ID1_PIN) << 1 | digitalRead(PLAYER_ID2_PIN);

    if (playerID == 0)
    {
        master_init();
    }
    else
    {
        //slave_init();
    }

    
}

void loop()
{
    //Handle Master tasks (USB Host controller side)
    if (playerID == 0)
    {
        master_task();
    }
    else
    {
        //slave_task();
    }

    if (usbd_xid.getType() != usbd_c[0].type)
    {
        Serial1.print("CHANGED CONTROLLER TYPE TO ");
        Serial1.println(usbd_c[0].type);
        usbd_xid.setType(usbd_c[0].type);
    }

    //Handle OG Xbox side (OG Xbox)
    static uint32_t poll_timer = 0;
    if (millis() - poll_timer > 4)
    {
        if(usbd_xid.getType() == DUKE)
        {
            UDCON &= ~(1 << DETACH);
            usbd_xid.sendReport(&usbd_c[0].duke.in, sizeof(usbd_duke_in_t));
            usbd_xid.getReport(&usbd_c[0].duke.out, sizeof(usbd_duke_out_t));
        }
        else if(usbd_xid.getType() == STEELBATTALTION)
        {
            UDCON &= ~(1 << DETACH);
            usbd_xid.sendReport(&usbd_c[0].sb.in, sizeof(usbd_sbattalion_in_t));
            usbd_xid.getReport(&usbd_c[0].sb.out, sizeof(usbd_sbattalion_out_t));
        }
        else if(usbd_xid.getType() == DISCONNECTED)
        {
            UDCON |= (1 << DETACH);
        }
        
        poll_timer = millis();
    }
}