// Copyright 2020, Ryan Wendland, ogx360
// SPDX-License-Identifier: GPL-3.0-or-later

#include <Arduino.h>
#include <Wire.h>

#include "main.h"

extern usbd_controller_t usbd_c[MAX_GAMEPADS];

void i2c_get_data(int len)
{
    uint8_t start = Wire.read();

    //0xAA is a ping to see if the slave module is connected
    //Flash the LED to confirm receipt.
    if (start[0] == 0xAA)
    {
        digitalWrite(ARDUINO_LED_PIN, LOW);
        delay(250);
        digitalWrite(ARDUINO_LED_PIN, HIGH);
        goto flush;
    }

    //Controller state packet 0xFx, where 'x' is the controller type.
    if (start & 0xF0 == 0xF0)
    {
        usbd_c.type = start & 0x0F;
        if (usbd_c.type == DISCONNECTED)
        {
            goto flush;
        }

        uint8_t *rxbuf = NULL;
        uint8_t rxlen = 0;
        if (usbd_c.type == DUKE)
        {
            if (len != usbd_c.duke.in + 1)
                goto flush;
            rxbuf = (uint8_t*)&usbd_c.duke.in;
            rxlen = sizeof(usbd_c.duke.in);
        }
        else if (usbd_c.type == STEELBATTALTION)
        {
            if (len != usbd_c.sb.in + 1)
                goto flush;
            rxbuf = (uint8_t*)&usbd_c.sb.in;
            rxlen = sizeof(usbd_c.sb.in);
        }
        for (uint8_t i = 0; i < rxlen; i++)
        {
            rxbuf[i] = Wire.read();
        }
    }

flush:
    Wire.flush();
}

void i2c_send_data(void)
{

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
