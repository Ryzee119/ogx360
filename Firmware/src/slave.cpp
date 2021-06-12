// Copyright 2021, Ryan Wendland, ogx360
// SPDX-License-Identifier: GPL-3.0-or-later

#include <Arduino.h>
#include <Wire.h>

#include "main.h"

extern usbd_controller_t usbd_c[MAX_GAMEPADS];

void i2c_get_data(int len)
{
    uint8_t packet_id = Wire.read();

    //0xAA is a ping to see if the slave module is connected
    //Flash the LED to confirm receipt.
    if (packet_id == 0xAA)
    {
        digitalWrite(ARDUINO_LED_PIN, LOW);
        delay(250);
        digitalWrite(ARDUINO_LED_PIN, HIGH);
        goto flush_and_leave;
    }

    //Controller state packet 0xFx, where 'x' is the controller type.
    if ((packet_id & 0xF0) == 0xF0)
    {
        usbd_c[0].type = (xid_type_t)(packet_id & 0x0F);

        uint8_t *rxbuf = (usbd_c[0].type == DUKE) ? ((uint8_t*)&usbd_c[0].duke.in) :
                         (usbd_c[0].type == STEELBATTALTION) ? ((uint8_t*)&usbd_c[0].sb.in) :
                         NULL;
        uint8_t  rxlen = (usbd_c[0].type == DUKE) ? sizeof(usbd_c[0].duke.in) :
                         (usbd_c[0].type == STEELBATTALTION) ? sizeof(usbd_c[0].sb.in) :
                         0;

        if (len != rxlen + 1 /* because of status byte */ || rxbuf == NULL || rxlen == 0)
        {
            goto flush_and_leave;
        }

        for (uint8_t i = 0; i < rxlen; i++)
        {
            rxbuf[i] = Wire.read();
        }
    }

flush_and_leave:
    while (Wire.available())
    {
        Wire.read();
    }
}

void i2c_send_data(void)
{
    if (usbd_c[0].type == DUKE)
    {
        Wire.write((uint8_t *)&usbd_c[0].duke.out, sizeof(usbd_c[0].duke.out));
    }
    else if (usbd_c[0].type == STEELBATTALTION)
    {
        Wire.write((uint8_t *)&usbd_c[0].sb.out, sizeof(usbd_c[0].sb.out));
    }
    else
    {
        //Just send something back so master isnt waiting
        Wire.write(0x00);
    }
}

void slave_init(void)
{
    uint8_t slave_id = digitalRead(PLAYER_ID1_PIN) << 1 | digitalRead(PLAYER_ID2_PIN);
    Wire.begin(slave_id);
    Wire.setClock(400000);
    Wire.onRequest(i2c_send_data);
    Wire.onReceive(i2c_get_data);
    Wire.setWireTimeout(4000, true);
}

void slave_task(void)
{
    //nothing to do!
}
