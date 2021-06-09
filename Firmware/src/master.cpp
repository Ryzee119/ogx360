// Copyright 2020, Ryan Wendland, ogx360
// SPDX-License-Identifier: GPL-3.0-or-later

#include <Arduino.h>
#include <Wire.h>
#include <EEPROM.h>
#include <UHS2/Usb.h>
#include <UHS2/usbhub.h>

#include "main.h"

USB UsbHost;
USBHub Hub(&UsbHost);
XINPUT xinput1(&UsbHost);
XINPUT xinput2(&UsbHost);
XINPUT xinput3(&UsbHost);
XINPUT xinput4(&UsbHost);

extern usbd_controller_t usbd_c[MAX_GAMEPADS];

static void handle_duke(usbh_xinput_t *_usbh_xinput, usbd_duke_t* _usbd_duke);
static void handle_sbattalion(usbh_xinput_t *_usbh_xinput, usbd_steelbattalion_t* _usbd_sbattalion);

void wire_send(uint8_t addr, const uint8_t *data, uint8_t len)
{
    Wire.beginTransmission(addr);
    Wire.write(data, len);
    Wire.endTransmission(true);
}

void master_init(void)
{
    pinMode(USB_HOST_RESET_PIN, OUTPUT);
    digitalWrite(USB_HOST_RESET_PIN, LOW);

    Serial1.begin(115200);
    Wire.begin();
    Wire.setClock(400000);

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

void master_task()
{
    UsbHost.Task();
    UsbHost.IntHandler();

    usbh_xinput_t *usbh_head = usbh_xinput_get_device_list();
    for (int i = 0; i < MAX_GAMEPADS; i++)
    {
        usbh_xinput_t *_usbh_xinput = &usbh_head[i];
        usbd_controller_t *_usbd_c = &usbd_c[i];
        usbd_duke_t *_usbd_duke = &_usbd_c->duke;
        usbd_steelbattalion_t *_usbd_sbattalion = &_usbd_c->sb;

        if (_usbh_xinput->bAddress == 0)
        {
            _usbd_c->type = DISCONNECTED;
        }

        //Must be connected, set a default device
        if (_usbh_xinput->bAddress && _usbd_c->type == DISCONNECTED)
        {
            _usbd_c->type = DUKE;
        }

        if (usbh_xinput_is_chatpad_pressed(_usbh_xinput, XINPUT_CHATPAD_GREEN))
        {
            _usbd_c->type = DUKE;
            _usbh_xinput->chatpad_led_requested = CHATPAD_GREEN;
        }
        else if (usbh_xinput_is_chatpad_pressed(_usbh_xinput, XINPUT_CHATPAD_ORANGE))
        {
            _usbd_c->type = STEELBATTALTION;
            _usbh_xinput->chatpad_led_requested = CHATPAD_ORANGE;
        }

        if (_usbd_c->type == DUKE)
        {
            handle_duke(_usbh_xinput, _usbd_duke);
        }
        else if (_usbd_c->type == STEELBATTALTION)
        {
            handle_sbattalion(_usbh_xinput, _usbd_sbattalion);
        }

        if (i == 0)
            continue; //We're done on this loop

        //Now send data to slaves
        uint8_t rx_len, tx_len;
        uint8_t *rx_buff, *tx_buff;

        if (_usbd_c->type == DUKE)
        {
            tx_len = sizeof(usbd_duke_in_t);
            tx_buff = (uint8_t *)&_usbd_duke->in;
            rx_len = sizeof(usbd_duke_out_t);
            rx_buff = (uint8_t *)&_usbd_duke->out;
        }
        else if (_usbd_c->type == STEELBATTALTION)
        {
            tx_len = sizeof(usbd_sbattalion_in_t);
            tx_buff = (uint8_t *)&_usbd_sbattalion->in;
            rx_len = sizeof(usbd_sbattalion_out_t);
            rx_buff = (uint8_t *)&_usbd_sbattalion->out;
        }

        Wire.beginTransmission(i);

        uint8_t status = 0xF0 | _usbd_c->type;
        Wire.write(&status, 1);

        if (_usbd_c->type == DISCONNECTED)
        {
            Wire.endTransmission(true);
            continue;
        }

        Wire.write(tx_buff, tx_len);
        Wire.endTransmission(true);
        /*
        if (Wire.requestFrom(i, (int)rx_len) == rx_len)
        {
            while (Wire.available())
            {
                // *rx_buff = Wire.read();
                //rx_buff++;
            }
        }
        else
        {
            Wire.flush();
        }
*/
    }
}

static void handle_duke(usbh_xinput_t *_usbh_xinput, usbd_duke_t* _usbd_duke)
{
    xinput_padstate_t *usbh_xstate = &_usbh_xinput->pad_state;
    memset(&_usbd_duke->in.wButtons, 0x00, 10);

    if (usbh_xstate->wButtons & XINPUT_GAMEPAD_DPAD_UP) _usbd_duke->in.wButtons |= DUKE_DUP;
    if (usbh_xstate->wButtons & XINPUT_GAMEPAD_DPAD_DOWN) _usbd_duke->in.wButtons |= DUKE_DDOWN;
    if (usbh_xstate->wButtons & XINPUT_GAMEPAD_DPAD_LEFT) _usbd_duke->in.wButtons |= DUKE_DLEFT;
    if (usbh_xstate->wButtons & XINPUT_GAMEPAD_DPAD_RIGHT) _usbd_duke->in.wButtons |= DUKE_DRIGHT;
    if (usbh_xstate->wButtons & XINPUT_GAMEPAD_START) _usbd_duke->in.wButtons |= DUKE_START;
    if (usbh_xstate->wButtons & XINPUT_GAMEPAD_BACK) _usbd_duke->in.wButtons |= DUKE_BACK;
    if (usbh_xstate->wButtons & XINPUT_GAMEPAD_LEFT_THUMB) _usbd_duke->in.wButtons |= DUKE_LS;
    if (usbh_xstate->wButtons & XINPUT_GAMEPAD_RIGHT_THUMB) _usbd_duke->in.wButtons |= DUKE_RS;

    //Analog buttons are converted to digital
    if (usbh_xstate->wButtons & XINPUT_GAMEPAD_LEFT_SHOULDER) _usbd_duke->in.WHITE = 0xFF;
    if (usbh_xstate->wButtons & XINPUT_GAMEPAD_RIGHT_SHOULDER) _usbd_duke->in.BLACK = 0xFF;
    if (usbh_xstate->wButtons & XINPUT_GAMEPAD_A) _usbd_duke->in.A = 0xFF;
    if (usbh_xstate->wButtons & XINPUT_GAMEPAD_B) _usbd_duke->in.B = 0xFF;
    if (usbh_xstate->wButtons & XINPUT_GAMEPAD_X) _usbd_duke->in.X = 0xFF;
    if (usbh_xstate->wButtons & XINPUT_GAMEPAD_Y) _usbd_duke->in.Y = 0xFF;

    //Analog Sticks
    _usbd_duke->in.leftStickX  = usbh_xstate->sThumbLX;
    _usbd_duke->in.leftStickY  = usbh_xstate->sThumbLY;
    _usbd_duke->in.rightStickX = usbh_xstate->sThumbRX;
    _usbd_duke->in.rightStickY = usbh_xstate->sThumbRY;
    _usbd_duke->in.L           = usbh_xstate->bLeftTrigger;
    _usbd_duke->in.R           = usbh_xstate->bRightTrigger;

    _usbh_xinput->chatpad_led_requested = CHATPAD_GREEN;
    _usbh_xinput->lValue_requested = _usbd_duke->out.lValue >> 8;
    _usbh_xinput->rValue_requested = _usbd_duke->out.hValue >> 8;
}

#define ONGAMEPAD 0
#define ONCHATPAD 1
/*
static const uint16_t sb_map[][5] PROGMEM =
{
    {ONGAMEPAD, XINPUT_GAMEPAD_START, SBC_W0_START, 0, 0},
    {ONCHATPAD, XINPUT_CHATPAD_ENTER, SBC_W0_START, 0, 0},

    {ONGAMEPAD, XINPUT_GAMEPAD_LEFT_SHOULDER, SBC_W0_RIGHTJOYFIRE, 0, 0},
    {ONGAMEPAD, XINPUT_GAMEPAD_LEFT_THUMB, SBC_W2_LEFTJOYSIGHTCHANGE, 2, 0}, //Hold for 500ms to centre sight

    {ONGAMEPAD, XINPUT_GAMEPAD_RIGHT_SHOULDER, SBC_W0_RIGHTJOYLOCKON, 0, 0},
    {ONGAMEPAD, XINPUT_GAMEPAD_B, SBC_W0_RIGHTJOYLOCKON, 0, 0},

    {ONGAMEPAD, XINPUT_GAMEPAD_RIGHT_SHOULDER, SBC_W0_RIGHTJOYMAINWEAPON, 0, 0},
    {ONGAMEPAD, XINPUT_GAMEPAD_A, SBC_W0_RIGHTJOYMAINWEAPON, 0, 0},

    {ONGAMEPAD, XINPUT_GAMEPAD_XBOX_BUTTON, SBC_W0_EJECT, 0, 0},
    {ONCHATPAD, XINPUT_CHATPAD_0, SBC_W0_EJECT, 1, 0},

    //Rumbles right motor for chaff
    //Rumbles both motors for Fire
    //Rumbles both for mag change
    //Rumbles both for Eject
    {ONGAMEPAD, XINPUT_GAMEPAD_X, SBC_W1_EXTINGUISHER, 1, 1}, //Only if Extinguisher light flashing
    {ONGAMEPAD, XINPUT_GAMEPAD_X, SBC_W1_WEAPONCONMAGAZINE, 1, 2}, //Only if Mag change light flashing
    {ONGAMEPAD, XINPUT_GAMEPAD_X, SBC_W1_WASHING, 1, 3}, //Only if washing light flashing
    {ONGAMEPAD, XINPUT_GAMEPAD_X, SBC_W0_EJECT, 0, 8}, //Only if eject light flashing

    {ONCHATPAD, XINPUT_CHATPAD_1, SBC_W1_COMM1, 1, 4}, //Only if holding XINPUT_CHATPAD_MESSENGER or XINPUT_GAMEPAD_BACK
    {ONCHATPAD, XINPUT_CHATPAD_2, SBC_W1_COMM2, 1, 4}, //Only if holding XINPUT_CHATPAD_MESSENGER or XINPUT_GAMEPAD_BACK
    {ONCHATPAD, XINPUT_CHATPAD_3, SBC_W1_COMM3, 1, 4}, //Only if holding XINPUT_CHATPAD_MESSENGER or XINPUT_GAMEPAD_BACK
    {ONCHATPAD, XINPUT_CHATPAD_4, SBC_W1_COMM4, 1, 4}, //Only if holding XINPUT_CHATPAD_MESSENGER or XINPUT_GAMEPAD_BACK
    {ONCHATPAD, XINPUT_CHATPAD_5, SBC_W2_COMM5, 2, 4}, //Only if holding XINPUT_CHATPAD_MESSENGER or XINPUT_GAMEPAD_BACK

    {ONCHATPAD, XINPUT_CHATPAD_1, SBC_W1_FUNCTIONF1, 1, 0},
    {ONCHATPAD, XINPUT_CHATPAD_2, SBC_W1_FUNCTIONTANKDETACH, 1, 0},
    {ONCHATPAD, XINPUT_CHATPAD_3, SBC_W0_FUNCTIONFSS, 0, 0},
    {ONCHATPAD, XINPUT_CHATPAD_4, SBC_W1_FUNCTIONF2, 1, 0},
    {ONCHATPAD, XINPUT_CHATPAD_5, SBC_W1_FUNCTIONOVERRIDE, 1, 0},
    {ONCHATPAD, XINPUT_CHATPAD_6, SBC_W0_FUNCTIONMANIPULATOR, 0, 0},
    {ONCHATPAD, XINPUT_CHATPAD_7, SBC_W1_FUNCTIONF3, 1, 0},
    {ONCHATPAD, XINPUT_CHATPAD_8, SBC_W1_FUNCTIONNIGHTSCOPE, 1, 0},
    {ONCHATPAD, XINPUT_CHATPAD_9, SBC_W0_FUNCTIONLINECOLORCHANGE, 0, 0},

    //If mod 4, DUP and DOWN changes tuner dial
    //If no mod, DUP, DOWN changes gear lever pos
    //{ONGAMEPAD, XINPUT_GAMEPAD_DPAD_UP, SBC_AXIS_AIMINGX, 0, 4},
    //{ONGAMEPAD, XINPUT_GAMEPAD_DPAD_UP, SBC_AXIS_AIMINGX, 0, 0},


    {ONCHATPAD, XINPUT_CHATPAD_Q, SBC_W2_TOGGLEOXYGENSUPPLY, 2, 5},
    {ONCHATPAD, XINPUT_CHATPAD_A, SBC_W2_TOGGLEOXYGENSUPPLY, 2, 5},
    {ONCHATPAD, XINPUT_CHATPAD_W, SBC_W2_TOGGLEOXYGENSUPPLY, 2, 5},
    {ONCHATPAD, XINPUT_CHATPAD_S, SBC_W2_TOGGLEOXYGENSUPPLY, 2, 5},
    {ONCHATPAD, XINPUT_CHATPAD_Z, SBC_W2_TOGGLEOXYGENSUPPLY, 2, 5},
    {ONCHATPAD, XINPUT_CHATPAD_D, SBC_W2_TOGGLEOXYGENSUPPLY, 2, 5},
    {ONCHATPAD, XINPUT_CHATPAD_F, SBC_W2_TOGGLEOXYGENSUPPLY, 2, 5},
    {ONCHATPAD, XINPUT_CHATPAD_SHIFT, SBC_W2_TOGGLEOXYGENSUPPLY, 2, 6},
    {ONCHATPAD, XINPUT_CHATPAD_G, SBC_W1_CHAFF, 1, 0},
    {ONCHATPAD, XINPUT_CHATPAD_Y, SBC_W1_CHAFF, 1, 0},
    {ONCHATPAD, XINPUT_CHATPAD_X, SBC_W1_WEAPONCONMAIN, 1, 0},
    {ONCHATPAD, XINPUT_CHATPAD_RIGHT, SBC_W1_WEAPONCONMAIN, 1, 0},
    {ONCHATPAD, XINPUT_CHATPAD_C, SBC_W1_WEAPONCONSUB, 1, 0},
    {ONCHATPAD, XINPUT_CHATPAD_LEFT, SBC_W1_WEAPONCONSUB, 1, 0},
    {ONCHATPAD, XINPUT_CHATPAD_V, SBC_W1_WEAPONCONMAGAZINE, 1, 0},
    {ONCHATPAD, XINPUT_CHATPAD_SPACE, SBC_W1_WEAPONCONMAGAZINE, 1, 0},
    {ONCHATPAD, XINPUT_CHATPAD_U, SBC_W0_MULTIMONOPENCLOSE, 0, 0},
    {ONCHATPAD, XINPUT_CHATPAD_J, SBC_W0_MULTIMONMODESELECT, 0, 0},
    {ONCHATPAD, XINPUT_CHATPAD_N, SBC_W0_MAINMONZOOMIN, 0, 0},
    {ONCHATPAD, XINPUT_CHATPAD_I, SBC_W0_MULTIMONMAPZOOMINOUT, 0, 0},
    {ONCHATPAD, XINPUT_CHATPAD_K, SBC_W0_MULTIMONSUBMONITOR, 0, 0},
    {ONCHATPAD, XINPUT_CHATPAD_M, SBC_W0_MAINMONZOOMOUT, 0, 0},
    {ONCHATPAD, XINPUT_CHATPAD_P, SBC_W0_COCKPITHATCH, 0, 7},
    {ONCHATPAD, XINPUT_CHATPAD_COMMA, SBC_W0_IGNITION, 0, 7}, //Mod 7. have to disable SBC_W0_COCKPITHATCH to fix IGR
};
*/
static void handle_sbattalion(usbh_xinput_t *_usbh_xinput, usbd_steelbattalion_t *_usbd_sbattalion)
{
    xinput_padstate_t *usbh_xstate = &_usbh_xinput->pad_state;
    _usbd_sbattalion->in.wButtons[0] = 0x0000;
    _usbd_sbattalion->in.wButtons[1] = 0x0000;
    _usbd_sbattalion->in.wButtons[2] &= 0xFFFC; //Dont clear toggle switches
                                                /*
    for (uint8_t i = 0; i < (sizeof(sb_map) / sizeof(sb_map[0])); i++)
    {
        uint16_t in_type  = pgm_read_word(&sb_map[i][0]);
        uint16_t in_map = pgm_read_word(&sb_map[i][1]);
        uint16_t out_map = pgm_read_word(&sb_map[i][2]);
        uint16_t out_word = pgm_read_word(&sb_map[i][3]);
        uint16_t mod = pgm_read_word(&sb_map[i][4]);

        
        uint8_t pressed = (in_type == ONGAMEPAD) ? usbh_xstate->wButtons & in_map : \
                                                   usbh_xinput_is_chatpad_pressed(_usbh_xinput, in_map);
        if (pressed) Serial1.println(pressed);
        switch (mod)
        {
        case 1: //Only if Extinguisher light flashing
            if (_usbd_sbattalion->out.Extinguisher)
                goto apply;
            break;
        case 2: //Only if Mag light flashing
            if (_usbd_sbattalion->out.MagazineChange)
                goto apply;
            break;
        case 3: //Only if washing light flashing
            if (_usbd_sbattalion->out.Washing)
                goto apply;
            break;   
        case 8: //Only if eject light flashing
            if (_usbd_sbattalion->out.EmergencyEject)
                goto apply;
            break;
        case 4: //Only if holding XINPUT_CHATPAD_MESSENGER or XINPUT_GAMEPAD_BACK
                //If mod 4, DUP and DOWN changes tuner dial
                //If no mod, DUP, DOWN changes gear lever pos
            if (usbh_xstate->wButtons & XINPUT_GAMEPAD_BACK || usbh_xinput_is_chatpad_pressed(_usbh_xinput, XINPUT_CHATPAD_MESSENGER))
                goto apply;
            break;
        case 5: //Mod 5 is click toggle
            break;
        case 6: //Mod 6 is clock toggle all switches
            break;
        case 7: //Mod 7. Cannt have to disable SBC_W0_COCKPITHATCH and SBC_W0_IGNITION or we trigger IGR
                _usbd_sbattalion->in.wButtons[0] &= ~SBC_W0_COCKPITHATCH;
                _usbd_sbattalion->in.wButtons[0] &= ~SBC_W0_IGNITION;
            break;
        apply:
        default:
            if (pressed)
            {
                Serial1.print("Pressed ");
                Serial1.println(in_map);
                Serial1.println(out_map);
                _usbd_sbattalion->in.wButtons[out_word] |= out_map;
            }
                
        }
    }
*/

    //R,N,1,2,3,4,5
    static const uint8_t gearStates[7] = {7, 8, 9, 10, 11, 12, 13};
    static int8_t currentGear = 1;
    static int32_t virtualMouseX = 32768, virtualMouseY = 32768;

    _usbd_sbattalion->in.wButtons[0] = 0x0000;
    _usbd_sbattalion->in.wButtons[1] = 0x0000;
    _usbd_sbattalion->in.wButtons[2] &= 0xFFFC; //Need to only clear the two LSBs. The other bits are the toggle switches

    //L1/R1 = L and R bumpers
    //L2/R2 = L and R Triggers
    //L3/R3 - L and R Stick Press
    //Note the W0,W1 or W2 in the SBC_GAMEPAD button defines the offset in dButtons[X].
    //i.e. SBC_W1_COMM3 should use dButtons[1].
    if (usbh_xstate->wButtons & XINPUT_GAMEPAD_START)
        _usbd_sbattalion->in.wButtons[0] |= SBC_W0_START;
    if (usbh_xstate->wButtons & XINPUT_GAMEPAD_LEFT_SHOULDER)
        _usbd_sbattalion->in.wButtons[0] |= SBC_W0_RIGHTJOYFIRE;
    static uint32_t L3HoldTimer = 0; //Timer for holding the Left stick in
    if (usbh_xstate->wButtons & XINPUT_GAMEPAD_LEFT_THUMB)
    {
        _usbd_sbattalion->in.wButtons[2] |= SBC_W2_LEFTJOYSIGHTCHANGE;
        if (L3HoldTimer == 0 && (virtualMouseY != 32768 || virtualMouseX != 32768))
        {
            L3HoldTimer = millis();
        }
        else if ((millis() - L3HoldTimer) > 500)
        {
            virtualMouseX = 32768;
            virtualMouseY = 32768;
            L3HoldTimer = 0;
        }
    }
    else
    {
        L3HoldTimer = 0;
    }

    if ((usbh_xstate->wButtons & XINPUT_GAMEPAD_RIGHT_THUMB) || (usbh_xstate->wButtons & XINPUT_GAMEPAD_B))
        _usbd_sbattalion->in.wButtons[0] |= SBC_W0_RIGHTJOYLOCKON;

    if ((usbh_xstate->wButtons & XINPUT_GAMEPAD_RIGHT_SHOULDER) || (usbh_xstate->wButtons & XINPUT_GAMEPAD_A))
        _usbd_sbattalion->in.wButtons[0] |= SBC_W0_RIGHTJOYMAINWEAPON;

    if ((usbh_xstate->wButtons & XINPUT_GAMEPAD_XBOX_BUTTON) || usbh_xinput_is_chatpad_pressed(_usbh_xinput, XINPUT_CHATPAD_0))
        _usbd_sbattalion->in.wButtons[0] |= SBC_W0_EJECT;

    //What the X button does depends on what is needed by your VT.
    //It will Extinguish, Reload (if empty), or Wash if required. It will rumble for Chaff but you need to press Y to chaff.
    //This is determined by reading back the LED feedback from the console. The game normally
    //makes these LEDs flash when action is required. I use this data to rumble the controller and determine what X should do.
    uint8_t SB_left_actuator = 0;
    uint8_t SB_right_actuator = 0;
    if (_usbd_sbattalion->out.Chaff != 0)
    {
        SB_right_actuator = (_usbd_sbattalion->out.Chaff << 4); //Only use right motor for chaff
    }
    if (_usbd_sbattalion->out.Extinguisher != 0)
    {
        SB_left_actuator = (_usbd_sbattalion->out.Extinguisher << 4);
        SB_right_actuator = SB_left_actuator;
        if (usbh_xstate->wButtons & XINPUT_GAMEPAD_X)
            _usbd_sbattalion->in.wButtons[1] |= SBC_W1_EXTINGUISHER;
    }
    if (_usbd_sbattalion->out.MagazineChange != 0)
    {
        SB_left_actuator = (_usbd_sbattalion->out.MagazineChange << 4);
        SB_right_actuator = SB_left_actuator;
        if (usbh_xstate->wButtons & XINPUT_GAMEPAD_X)
            _usbd_sbattalion->in.wButtons[1] |= SBC_W1_WEAPONCONMAGAZINE;
    }
    if (_usbd_sbattalion->out.Washing != 0)
    {
        if ((usbh_xstate->wButtons & XINPUT_GAMEPAD_X))
            _usbd_sbattalion->in.wButtons[1] |= SBC_W1_WASHING;
    }
    if (_usbd_sbattalion->out.EmergencyEject != 0)
    {
        SB_left_actuator = _usbd_sbattalion->out.EmergencyEject << 4;
        SB_right_actuator = SB_left_actuator;
    }

    _usbh_xinput->lValue_requested = SB_left_actuator;
    _usbh_xinput->lValue_requested = SB_right_actuator;

    //Hold the messenger button for COMMS and Adjust TunerDial
    if (usbh_xinput_is_chatpad_pressed(_usbh_xinput, XINPUT_CHATPAD_MESSENGER) || (usbh_xstate->wButtons & XINPUT_GAMEPAD_BACK))
    {
        if (usbh_xinput_is_chatpad_pressed(_usbh_xinput, XINPUT_CHATPAD_1))
            _usbd_sbattalion->in.wButtons[1] |= SBC_W1_COMM1;
        if (usbh_xinput_is_chatpad_pressed(_usbh_xinput, XINPUT_CHATPAD_2))
            _usbd_sbattalion->in.wButtons[1] |= SBC_W1_COMM2;
        if (usbh_xinput_is_chatpad_pressed(_usbh_xinput, XINPUT_CHATPAD_3))
            _usbd_sbattalion->in.wButtons[1] |= SBC_W1_COMM3;
        if (usbh_xinput_is_chatpad_pressed(_usbh_xinput, XINPUT_CHATPAD_4))
            _usbd_sbattalion->in.wButtons[1] |= SBC_W1_COMM4;
        if (usbh_xinput_is_chatpad_pressed(_usbh_xinput, XINPUT_CHATPAD_5))
            _usbd_sbattalion->in.wButtons[2] |= SBC_W2_COMM5;

        //Change tuner dial position by Holding the messenger then pressing D-pad directions.
        //Tuner dial = 0-15, corresponding to the 9o'clock position going clockwise.
        if (usbh_xinput_was_gamepad_pressed(_usbh_xinput, XINPUT_GAMEPAD_DPAD_UP) ||
            usbh_xinput_was_gamepad_pressed(_usbh_xinput, XINPUT_GAMEPAD_DPAD_RIGHT))
            _usbd_sbattalion->in.tunerDial += 2;

        if (usbh_xinput_was_gamepad_pressed(_usbh_xinput, XINPUT_GAMEPAD_DPAD_DOWN) ||
            usbh_xinput_was_gamepad_pressed(_usbh_xinput, XINPUT_GAMEPAD_DPAD_LEFT))
            _usbd_sbattalion->in.tunerDial = 2;

        if (_usbd_sbattalion->in.tunerDial > 15)
            _usbd_sbattalion->in.tunerDial = 15;
        if (_usbd_sbattalion->in.tunerDial < 0)
            _usbd_sbattalion->in.tunerDial = 0;

        //The default configuration
    }
    else if (!usbh_xinput_is_chatpad_pressed(_usbh_xinput, XINPUT_CHATPAD_ORANGE))
    {
        if (usbh_xinput_is_chatpad_pressed(_usbh_xinput, XINPUT_CHATPAD_1))
            _usbd_sbattalion->in.wButtons[1] |= SBC_W1_FUNCTIONF1;
        if (usbh_xinput_is_chatpad_pressed(_usbh_xinput, XINPUT_CHATPAD_2))
            _usbd_sbattalion->in.wButtons[1] |= SBC_W1_FUNCTIONTANKDETACH;
        if (usbh_xinput_is_chatpad_pressed(_usbh_xinput, XINPUT_CHATPAD_3))
            _usbd_sbattalion->in.wButtons[0] |= SBC_W0_FUNCTIONFSS;
        if (usbh_xinput_is_chatpad_pressed(_usbh_xinput, XINPUT_CHATPAD_4))
            _usbd_sbattalion->in.wButtons[1] |= SBC_W1_FUNCTIONF2;
        if (usbh_xinput_is_chatpad_pressed(_usbh_xinput, XINPUT_CHATPAD_5))
            _usbd_sbattalion->in.wButtons[1] |= SBC_W1_FUNCTIONOVERRIDE;
        if (usbh_xinput_is_chatpad_pressed(_usbh_xinput, XINPUT_CHATPAD_6))
            _usbd_sbattalion->in.wButtons[0] |= SBC_W0_FUNCTIONMANIPULATOR;
        if (usbh_xinput_is_chatpad_pressed(_usbh_xinput, XINPUT_CHATPAD_7))
            _usbd_sbattalion->in.wButtons[1] |= SBC_W1_FUNCTIONF3;
        if (usbh_xinput_is_chatpad_pressed(_usbh_xinput, XINPUT_CHATPAD_8))
            _usbd_sbattalion->in.wButtons[1] |= SBC_W1_FUNCTIONNIGHTSCOPE;
        if (usbh_xinput_is_chatpad_pressed(_usbh_xinput, XINPUT_CHATPAD_9))
            _usbd_sbattalion->in.wButtons[0] |= SBC_W0_FUNCTIONLINECOLORCHANGE;

        //Change gears by Pressing DUP or DDOWN. Limits are 0-6. //R,N,1,2,3,4,5
        //To prevent accidentally changing gears whilst rotating, I check to make sure you aren't pressing LEFT or RIGHT.
        if (!usbh_xinput_is_gamepad_pressed(_usbh_xinput, XINPUT_GAMEPAD_DPAD_LEFT) &&
            !usbh_xinput_is_gamepad_pressed(_usbh_xinput, XINPUT_GAMEPAD_DPAD_RIGHT))
        {
            if (usbh_xinput_was_gamepad_pressed(_usbh_xinput, XINPUT_GAMEPAD_DPAD_UP))
                currentGear++;
            if (usbh_xinput_was_gamepad_pressed(_usbh_xinput, XINPUT_GAMEPAD_DPAD_DOWN))
                currentGear--;
        }

        if (currentGear > 6)
            currentGear = 6;
        if (currentGear < 0)
            currentGear = 0;
        _usbd_sbattalion->in.gearLever = gearStates[currentGear];
    }

    if (usbh_xinput_is_chatpad_pressed(_usbh_xinput, XINPUT_CHATPAD_D))
        _usbd_sbattalion->in.wButtons[1] |= SBC_W1_WASHING;
    if (usbh_xinput_is_chatpad_pressed(_usbh_xinput, XINPUT_CHATPAD_F))
        _usbd_sbattalion->in.wButtons[1] |= SBC_W1_EXTINGUISHER;
    if (usbh_xinput_was_chatpad_pressed(_usbh_xinput, XINPUT_CHATPAD_Q))
        _usbd_sbattalion->in.wButtons[2] ^= SBC_W2_TOGGLEOXYGENSUPPLY; //Toggle Switch
    if (usbh_xinput_was_chatpad_pressed(_usbh_xinput, XINPUT_CHATPAD_A))
        _usbd_sbattalion->in.wButtons[2] ^= SBC_W2_TOGGLEFILTERCONTROL; //Toggle Switch
    if (usbh_xinput_was_chatpad_pressed(_usbh_xinput, XINPUT_CHATPAD_W))
        _usbd_sbattalion->in.wButtons[2] ^= SBC_W2_TOGGLEVTLOCATION; //Toggle Switch
    if (usbh_xinput_was_chatpad_pressed(_usbh_xinput, XINPUT_CHATPAD_S))
        _usbd_sbattalion->in.wButtons[2] ^= SBC_W2_TOGGLEBUFFREMATERIAL; //Toggle Switch
    if (usbh_xinput_was_chatpad_pressed(_usbh_xinput, XINPUT_CHATPAD_Z))
        _usbd_sbattalion->in.wButtons[2] ^= SBC_W2_TOGGLEFUELFLOWRATE; //Toggle Switch    
    if (usbh_xinput_was_chatpad_pressed(_usbh_xinput, XINPUT_CHATPAD_SHIFT))
    {
        if (_usbd_sbattalion->in.wButtons[2] &= 0xFFFC)
        { //If any of the toggle switches are on SHIFT will turn everything off.
            _usbd_sbattalion->in.wButtons[2] &= ~0xFFFC;
        }
        else
        {
            _usbd_sbattalion->in.wButtons[2] |= 0xFFFC; //If all toggle switches are OFF, this will quickly turn them all on
        }
    }

    if (usbh_xinput_is_chatpad_pressed(_usbh_xinput, XINPUT_CHATPAD_G) || (usbh_xstate->wButtons & XINPUT_GAMEPAD_Y))
        _usbd_sbattalion->in.wButtons[1] |= SBC_W1_CHAFF;

    if (usbh_xinput_is_chatpad_pressed(_usbh_xinput, XINPUT_CHATPAD_X) || usbh_xinput_is_chatpad_pressed(_usbh_xinput, XINPUT_CHATPAD_RIGHT))
        _usbd_sbattalion->in.wButtons[1] |= SBC_W1_WEAPONCONMAIN;

    if (usbh_xinput_is_chatpad_pressed(_usbh_xinput, XINPUT_CHATPAD_C) || usbh_xinput_is_chatpad_pressed(_usbh_xinput, XINPUT_CHATPAD_LEFT))
        _usbd_sbattalion->in.wButtons[1] |= SBC_W1_WEAPONCONSUB;

    if (usbh_xinput_is_chatpad_pressed(_usbh_xinput, XINPUT_CHATPAD_V) || usbh_xinput_is_chatpad_pressed(_usbh_xinput, XINPUT_CHATPAD_SPACE))
        _usbd_sbattalion->in.wButtons[1] |= SBC_W1_WEAPONCONMAGAZINE;

    if (usbh_xinput_is_chatpad_pressed(_usbh_xinput, XINPUT_CHATPAD_U))
        _usbd_sbattalion->in.wButtons[0] |= SBC_W0_MULTIMONOPENCLOSE;
    if (usbh_xinput_is_chatpad_pressed(_usbh_xinput, XINPUT_CHATPAD_J))
        _usbd_sbattalion->in.wButtons[0] |= SBC_W0_MULTIMONMODESELECT;
    if (usbh_xinput_is_chatpad_pressed(_usbh_xinput, XINPUT_CHATPAD_N))
        _usbd_sbattalion->in.wButtons[0] |= SBC_W0_MAINMONZOOMIN;
    if (usbh_xinput_is_chatpad_pressed(_usbh_xinput, XINPUT_CHATPAD_I))
        _usbd_sbattalion->in.wButtons[0] |= SBC_W0_MULTIMONMAPZOOMINOUT;
    if (usbh_xinput_is_chatpad_pressed(_usbh_xinput, XINPUT_CHATPAD_K))
        _usbd_sbattalion->in.wButtons[0] |= SBC_W0_MULTIMONSUBMONITOR;
    if (usbh_xinput_is_chatpad_pressed(_usbh_xinput, XINPUT_CHATPAD_M))
        _usbd_sbattalion->in.wButtons[0] |= SBC_W0_MAINMONZOOMOUT;
    if (usbh_xinput_is_chatpad_pressed(_usbh_xinput, XINPUT_CHATPAD_ENTER))
        _usbd_sbattalion->in.wButtons[0] |= SBC_W0_START;

    if (usbh_xinput_is_chatpad_pressed(_usbh_xinput, XINPUT_CHATPAD_P))
    {
        _usbd_sbattalion->in.wButtons[0] |= SBC_W0_COCKPITHATCH;
        _usbd_sbattalion->in.wButtons[0] &= ~SBC_W0_IGNITION; //Cannot have these two buttons pressed at the same time, some bioses will trigger an IGR
    }

    if (usbh_xinput_is_chatpad_pressed(_usbh_xinput, XINPUT_CHATPAD_COMMA))
    {
        _usbd_sbattalion->in.wButtons[0] |= SBC_W0_IGNITION;
        _usbd_sbattalion->in.wButtons[0] &= ~SBC_W0_COCKPITHATCH; //Cannot have these two buttons pressed at the same time, some bioses will trigger an IGR
    }

    //Apply Pedals
    _usbd_sbattalion->in.leftPedal = (uint16_t)(usbh_xstate->bLeftTrigger << 8);   //0x00 to 0xFF00 SIDESTEP PEDAL
    _usbd_sbattalion->in.rightPedal = (uint16_t)(usbh_xstate->bRightTrigger << 8); //0x00 to 0xFF00 ACCEL PEDAL
    if (usbh_xinput_is_chatpad_pressed(_usbh_xinput, XINPUT_CHATPAD_BACK))
    {
        _usbd_sbattalion->in.middlePedal = 0xFF00; //Brake Pedal
    }
    else
    {
        _usbd_sbattalion->in.middlePedal = 0x0000; //Brake Pedal
    }

    if (!usbh_xinput_is_chatpad_pressed(_usbh_xinput, XINPUT_CHATPAD_MESSENGER) && !(usbh_xstate->wButtons & XINPUT_GAMEPAD_BACK))
    {
        if ((usbh_xstate->wButtons & XINPUT_GAMEPAD_DPAD_LEFT))
        {
            _usbd_sbattalion->in.rotationLever = -32767;
        }
        else if ((usbh_xstate->wButtons & XINPUT_GAMEPAD_DPAD_RIGHT))
        {
            _usbd_sbattalion->in.rotationLever = +32767;
        }
        else
        {
            _usbd_sbattalion->in.rotationLever = 0;
        }
    }

    //Apply analog sticks
    static int32_t sensitivity = 400;
    EEPROM.get(0x00, sensitivity);
    if (usbh_xinput_is_chatpad_pressed(_usbh_xinput, XINPUT_CHATPAD_ORANGE))
    {
        if (usbh_xinput_is_chatpad_pressed(_usbh_xinput, XINPUT_CHATPAD_9))
            sensitivity = 200;
        if (usbh_xinput_is_chatpad_pressed(_usbh_xinput, XINPUT_CHATPAD_8))
            sensitivity = 250;
        if (usbh_xinput_is_chatpad_pressed(_usbh_xinput, XINPUT_CHATPAD_7))
            sensitivity = 300;
        if (usbh_xinput_is_chatpad_pressed(_usbh_xinput, XINPUT_CHATPAD_6))
            sensitivity = 350;
        if (usbh_xinput_is_chatpad_pressed(_usbh_xinput, XINPUT_CHATPAD_5))
            sensitivity = 400;
        if (usbh_xinput_is_chatpad_pressed(_usbh_xinput, XINPUT_CHATPAD_4))
            sensitivity = 650;
        if (usbh_xinput_is_chatpad_pressed(_usbh_xinput, XINPUT_CHATPAD_3))
            sensitivity = 800;
        if (usbh_xinput_is_chatpad_pressed(_usbh_xinput, XINPUT_CHATPAD_2))
            sensitivity = 1000;
        if (usbh_xinput_is_chatpad_pressed(_usbh_xinput, XINPUT_CHATPAD_1))
            sensitivity = 1200;
        int32_t temp = 0;
        EEPROM.get(0x00, temp);
        if (temp != sensitivity)
        {
            EEPROM.put(0x00, sensitivity);
            digitalWrite(ARDUINO_LED_PIN, !digitalRead(ARDUINO_LED_PIN));
            delay(100);
        }
    }

    _usbd_sbattalion->in.sightChangeX = _usbh_xinput->pad_state.sThumbLX;
    _usbd_sbattalion->in.sightChangeY = -_usbh_xinput->pad_state.sThumbLY - 1;

    if (!usbh_xinput_is_chatpad_pressed(_usbh_xinput, XINPUT_CHATPAD_MESSENGER) && !(usbh_xstate->wButtons & XINPUT_GAMEPAD_BACK))
    {
        //Moving aiming stick like a mouse cursor

        int32_t axisVal = _usbh_xinput->pad_state.sThumbRX;
        if (axisVal > 7500 || axisVal < -7500)
        {
            virtualMouseX += axisVal / sensitivity;
        }

        axisVal = _usbh_xinput->pad_state.sThumbRY;
        if (axisVal > 7500 || axisVal < -7500)
        {
            virtualMouseY -= axisVal / sensitivity;
        }

        if (virtualMouseX < 0)
            virtualMouseX = 0;
        if (virtualMouseX > 65535)
            virtualMouseX = 65535;
        if (virtualMouseY > 65535)
            virtualMouseY = 65535;
        if (virtualMouseY < 0)
            virtualMouseY = 0;

        _usbd_sbattalion->in.aimingX = (uint16_t)virtualMouseX;
        _usbd_sbattalion->in.aimingY = (uint16_t)virtualMouseY;
    }
}
