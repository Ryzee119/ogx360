// Copyright 2021, Ryan Wendland, ogx360
// SPDX-License-Identifier: GPL-3.0-or-later

#include <Arduino.h>
#include <Wire.h>
#include <UHS2/Usb.h>
#include <UHS2/usbhub.h>

#include "main.h"

USB UsbHost;
USBHub Hub(&UsbHost);
USBHub Hub1(&UsbHost);
USBHub Hub2(&UsbHost);
USBHub Hub3(&UsbHost);
USBHub Hub4(&UsbHost);
XINPUT xinput1(&UsbHost);
XINPUT xinput2(&UsbHost);
XINPUT xinput3(&UsbHost);
XINPUT xinput4(&UsbHost);

typedef struct xinput_user_data
{
    uint8_t modifiers;
    uint32_t button_hold_timer;
    int32_t vmouse_x;
    int32_t vmouse_y;
} xinput_user_data_t;

extern usbd_controller_t usbd_c[MAX_GAMEPADS];
xinput_user_data_t user_data[MAX_GAMEPADS];

static void handle_duke(usbh_xinput_t *_usbh_xinput, usbd_duke_t* _usbd_duke, xinput_user_data_t *_user_data);
static void handle_sbattalion(usbh_xinput_t *_usbh_xinput, usbd_steelbattalion_t* _usbd_sbattalion, xinput_user_data_t *_user_data);

void master_init(void)
{
    pinMode(USB_HOST_RESET_PIN, OUTPUT);
    digitalWrite(USB_HOST_RESET_PIN, LOW);

    Wire.begin();
    Wire.setClock(400000);
    Wire.setWireTimeout(4000, true);

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

    for (uint8_t i = 0; i < MAX_GAMEPADS; i++)
    {
        usbd_c[i].sb.in.gearLever = SBC_GEAR_N;
        user_data[i].vmouse_x = SBC_AIMING_MID;
        user_data[i].vmouse_y = SBC_AIMING_MID;
    }
}

void master_task(void)
{
    UsbHost.Task();
    UsbHost.IntHandler();

    usbh_xinput_t *usbh_head = usbh_xinput_get_device_list();
    for (int i = 0; i < MAX_GAMEPADS; i++)
    {
        usbh_xinput_t *_usbh_xinput = &usbh_head[i];
        usbd_controller_t *_usbd_c = &usbd_c[i];
        usbd_duke_t *_usbd_duke = &_usbd_c->duke;
        xinput_user_data_t *_user_data = &user_data[i];
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
            handle_duke(_usbh_xinput, _usbd_duke, _user_data);
        }
        else if (_usbd_c->type == STEELBATTALTION)
        {
            handle_sbattalion(_usbh_xinput, _usbd_sbattalion, _user_data);
        }

        if (i == 0)
        {
            continue;
        }

        //Now send data to slaves
        uint8_t *tx_buff = (_usbd_c->type == DUKE)            ? ((uint8_t *)&_usbd_duke->in) :
                           (_usbd_c->type == STEELBATTALTION) ? ((uint8_t *)&_usbd_sbattalion->in) :
                           NULL;
        uint8_t  tx_len =  (_usbd_c->type == DUKE)            ? sizeof(usbd_duke_in_t) :
                           (_usbd_c->type == STEELBATTALTION) ? sizeof(usbd_sbattalion_in_t) :
                           0;
        uint8_t *rx_buff = (_usbd_c->type == DUKE)            ? ((uint8_t *)&_usbd_duke->out) :
                           (_usbd_c->type == STEELBATTALTION) ? ((uint8_t *)&_usbd_sbattalion->out) :
                           NULL;
        uint8_t  rx_len =  (_usbd_c->type == DUKE)            ? sizeof(usbd_duke_out_t) :
                           (_usbd_c->type == STEELBATTALTION) ? sizeof(usbd_sbattalion_out_t) :
                           0;
        uint8_t status = 0xF0 | _usbd_c->type;

        Wire.beginTransmission(i);
        Wire.write(status);
        if (tx_buff != NULL && tx_len != 0)
        {
            Wire.write(tx_buff, tx_len);
        }
        Wire.endTransmission(true);

        if (rx_buff != NULL && rx_len != 0)
        {
            if (Wire.requestFrom(i, (int)rx_len) == rx_len)
            {
                while (Wire.available())
                {
                    *rx_buff = Wire.read();
                    rx_buff++;
                }
            }
        }
        //Flush
        while (Wire.available())
        {
            Wire.read();
        }
    }
}

static void handle_duke(usbh_xinput_t *_usbh_xinput, usbd_duke_t* _usbd_duke, xinput_user_data_t *_user_data)
{
    xinput_padstate_t *usbh_xstate = &_usbh_xinput->pad_state;
    memset(&_usbd_duke->in.wButtons, 0x00, 8); //Also clears A/B/X/Y,BLACK,WHITE

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

#define XINPUT_MOD_RSX_INVERT (1 << 0)
#define XINPUT_MOD_RSY_INVERT (1 << 1)
    if (_user_data->modifiers & XINPUT_MOD_RSY_INVERT) _usbd_duke->in.rightStickY = -usbh_xstate->sThumbRY - 1;
    if (_user_data->modifiers & XINPUT_MOD_RSX_INVERT) _usbd_duke->in.rightStickX = -usbh_xstate->sThumbRX - 1;
    
    if (usbh_xstate->wButtons & XINPUT_GAMEPAD_RIGHT_THUMB)
    {
        if(usbh_xinput_was_gamepad_pressed(_usbh_xinput, XINPUT_GAMEPAD_DPAD_UP) ||
           usbh_xinput_was_gamepad_pressed(_usbh_xinput, XINPUT_GAMEPAD_DPAD_DOWN))
        {
            (_user_data->modifiers & XINPUT_MOD_RSY_INVERT) ? _user_data->modifiers &= ~XINPUT_MOD_RSY_INVERT :
                                                              _user_data->modifiers |=  XINPUT_MOD_RSY_INVERT;
        }
        if(usbh_xinput_was_gamepad_pressed(_usbh_xinput, XINPUT_GAMEPAD_DPAD_RIGHT) ||
           usbh_xinput_was_gamepad_pressed(_usbh_xinput, XINPUT_GAMEPAD_DPAD_LEFT))
        {
            (_user_data->modifiers & XINPUT_MOD_RSX_INVERT) ? _user_data->modifiers &= ~XINPUT_MOD_RSX_INVERT :
                                                              _user_data->modifiers |=  XINPUT_MOD_RSX_INVERT;
        }
    }
}

typedef struct __attribute__((packed))
{
    uint16_t xinput_mask;
    uint16_t sb_mask;
    uint8_t sb_word_offset;
} sb_map_t;

//Mappings directly applied from gamepad
static const sb_map_t sb_pad_map [] PROGMEM =
{
    {XINPUT_GAMEPAD_START, SBC_W0_START, 0},
    {XINPUT_GAMEPAD_LEFT_SHOULDER, SBC_W0_RIGHTJOYFIRE, 0},
    {XINPUT_GAMEPAD_RIGHT_THUMB, SBC_W0_RIGHTJOYLOCKON, 0},
    {XINPUT_GAMEPAD_B, SBC_W0_RIGHTJOYLOCKON, 0},
    {XINPUT_GAMEPAD_RIGHT_SHOULDER, SBC_W0_RIGHTJOYMAINWEAPON, 0},
    {XINPUT_GAMEPAD_A, SBC_W0_RIGHTJOYMAINWEAPON, 0},
    {XINPUT_GAMEPAD_XBOX_BUTTON, SBC_W0_EJECT, 0},
    {XINPUT_GAMEPAD_LEFT_THUMB, SBC_W2_LEFTJOYSIGHTCHANGE, 2},
    {XINPUT_GAMEPAD_Y, SBC_W1_CHAFF, 1}
};

//Mappings directly applied from chatpad
static const sb_map_t sb_chatpad_map [] PROGMEM =
{
    {XINPUT_CHATPAD_0, SBC_W0_EJECT, 0},
    {XINPUT_CHATPAD_D, SBC_W1_WASHING, 1},
    {XINPUT_CHATPAD_F, SBC_W1_EXTINGUISHER, 1},
    {XINPUT_CHATPAD_G, SBC_W1_CHAFF, 1},
    {XINPUT_CHATPAD_X, SBC_W1_WEAPONCONMAIN, 1},
    {XINPUT_CHATPAD_RIGHT, SBC_W1_WEAPONCONMAIN, 1},
    {XINPUT_CHATPAD_C, SBC_W1_WEAPONCONSUB, 1},
    {XINPUT_CHATPAD_LEFT, SBC_W1_WEAPONCONSUB, 1},
    {XINPUT_CHATPAD_V, SBC_W1_WEAPONCONMAGAZINE, 1},
    {XINPUT_CHATPAD_SPACE, SBC_W1_WEAPONCONMAGAZINE, 1},
    {XINPUT_CHATPAD_U, SBC_W0_MULTIMONOPENCLOSE, 0},
    {XINPUT_CHATPAD_J, SBC_W0_MULTIMONMODESELECT, 0},
    {XINPUT_CHATPAD_N, SBC_W0_MAINMONZOOMIN, 0},
    {XINPUT_CHATPAD_I, SBC_W0_MULTIMONMAPZOOMINOUT, 0},
    {XINPUT_CHATPAD_K, SBC_W0_MULTIMONSUBMONITOR, 0},
    {XINPUT_CHATPAD_M, SBC_W0_MAINMONZOOMOUT, 0},
    {XINPUT_CHATPAD_ENTER, SBC_W0_START, 0},
    {XINPUT_CHATPAD_P, SBC_W0_COCKPITHATCH, 0},
    {XINPUT_CHATPAD_COMMA, SBC_W0_IGNITION, 0}
};

//Mappings only applied from chatpad when ALT button is held.
static const sb_map_t sb_chatpad_alt1_map [] PROGMEM =
{
    {XINPUT_CHATPAD_1, SBC_W1_COMM1, 1},
    {XINPUT_CHATPAD_2, SBC_W1_COMM2, 1},
    {XINPUT_CHATPAD_3, SBC_W1_COMM3, 1},
    {XINPUT_CHATPAD_4, SBC_W1_COMM4, 1},
    {XINPUT_CHATPAD_5, SBC_W2_COMM5, 2}
};

//Mappings only applied from chatpad when ALT button is NOT held.
static const sb_map_t sb_chatpad_alt2_map [] PROGMEM =
{
    {XINPUT_CHATPAD_1, SBC_W1_FUNCTIONF1, 1},
    {XINPUT_CHATPAD_2, SBC_W1_FUNCTIONTANKDETACH, 1},
    {XINPUT_CHATPAD_3, SBC_W0_FUNCTIONFSS, 0},
    {XINPUT_CHATPAD_4, SBC_W1_FUNCTIONF2, 1},
    {XINPUT_CHATPAD_5, SBC_W1_FUNCTIONOVERRIDE, 1},
    {XINPUT_CHATPAD_6, SBC_W0_FUNCTIONMANIPULATOR, 0},
    {XINPUT_CHATPAD_7, SBC_W1_FUNCTIONF3, 1},
    {XINPUT_CHATPAD_8, SBC_W1_FUNCTIONNIGHTSCOPE, 1},
    {XINPUT_CHATPAD_9, SBC_W0_FUNCTIONLINECOLORCHANGE, 0}
};

//Mappings from chatpad that are toggle switches
static const sb_map_t sb_chatpad_toggle_map [] PROGMEM =
{
    {XINPUT_CHATPAD_Q, SBC_W2_TOGGLEOXYGENSUPPLY, 2},
    {XINPUT_CHATPAD_A, SBC_W2_TOGGLEFILTERCONTROL, 2},
    {XINPUT_CHATPAD_W, SBC_W2_TOGGLEVTLOCATION, 2},
    {XINPUT_CHATPAD_S, SBC_W2_TOGGLEBUFFREMATERIAL, 2},
    {XINPUT_CHATPAD_Z, SBC_W2_TOGGLEFUELFLOWRATE, 2}
};

static void handle_sbattalion(usbh_xinput_t *_usbh_xinput, usbd_steelbattalion_t *_usbd_sbattalion, xinput_user_data_t *_user_data)
{
    xinput_padstate_t *usbh_xstate = &_usbh_xinput->pad_state;
    _usbd_sbattalion->in.wButtons[0] = 0x0000;
    _usbd_sbattalion->in.wButtons[1] = 0x0000;
    _usbd_sbattalion->in.wButtons[2] &= 0xFFFC; //Dont clear toggle switches

    //Apply gamepad direct mappings
    for (uint8_t i = 0; i < (sizeof(sb_pad_map) / sizeof(sb_pad_map[0])); i++)
    {   
        if (usbh_xstate->wButtons & pgm_read_word(&sb_pad_map[i].xinput_mask))
            _usbd_sbattalion->in.wButtons[pgm_read_byte(&sb_pad_map[i].sb_word_offset)] |= pgm_read_word(&sb_pad_map[i].sb_mask);
    }
    
    //Apply chatpad direct mappings
    for (uint8_t i = 0; i < (sizeof(sb_chatpad_map) / sizeof(sb_chatpad_map[0])); i++)
    {
        if (usbh_xinput_is_chatpad_pressed(_usbh_xinput, pgm_read_word(&sb_chatpad_map[i].xinput_mask)))
            _usbd_sbattalion->in.wButtons[pgm_read_byte(&sb_chatpad_map[i].sb_word_offset)] |= pgm_read_word(&sb_chatpad_map[i].sb_mask);
    }

    //Apply chatpad toggle switch mappings
    for (uint8_t i = 0; i < (sizeof(sb_chatpad_toggle_map) / sizeof(sb_chatpad_toggle_map[0])); i++)
    {
        if (usbh_xinput_was_chatpad_pressed(_usbh_xinput, pgm_read_word(&sb_chatpad_toggle_map[i].xinput_mask)))
            _usbd_sbattalion->in.wButtons[pgm_read_byte(&sb_chatpad_toggle_map[i].sb_word_offset)] ^=
                pgm_read_word(&sb_chatpad_toggle_map[i].sb_mask);
    }

    //What the X button does depends on what is needed by your VT.
    if (usbh_xstate->wButtons & XINPUT_GAMEPAD_X && (_usbd_sbattalion->out.Chaff_Extinguisher & 0x0F) != 0)
        _usbd_sbattalion->in.wButtons[1] |= SBC_W1_EXTINGUISHER;

    if (usbh_xstate->wButtons & XINPUT_GAMEPAD_X && (_usbd_sbattalion->out.Comm1_MagazineChange & 0x0F) != 0)
        _usbd_sbattalion->in.wButtons[1] |= SBC_W1_WEAPONCONMAGAZINE;

    if (usbh_xstate->wButtons & XINPUT_GAMEPAD_X && (_usbd_sbattalion->out.Washing_LineColorChange & 0xF0) != 0)
        _usbd_sbattalion->in.wButtons[1] |= SBC_W1_WASHING;

    //Hold the messenger button for COMMS and Adjust TunerDial
    if (usbh_xinput_is_chatpad_pressed(_usbh_xinput, XINPUT_CHATPAD_MESSENGER) || (usbh_xstate->wButtons & XINPUT_GAMEPAD_BACK))
    {
        //Apply chatpad alt1 mappings
        for (uint8_t i = 0; i < (sizeof(sb_chatpad_alt1_map) / sizeof(sb_chatpad_alt1_map[0])); i++)
        {
            if (usbh_xinput_is_chatpad_pressed(_usbh_xinput, pgm_read_word(&sb_chatpad_alt1_map[i].xinput_mask)))
                _usbd_sbattalion->in.wButtons[pgm_read_byte(&sb_chatpad_alt1_map[i].sb_word_offset)] |=
                    pgm_read_word(&sb_chatpad_alt1_map[i].sb_mask);
        }

        //Change tuner dial position by Holding the messenger then pressing D-pad directions.
        //Tuner dial = 0-15, corresponding to the 9o'clock position going clockwise.
        if (usbh_xinput_was_gamepad_pressed(_usbh_xinput, XINPUT_GAMEPAD_DPAD_UP) ||
            usbh_xinput_was_gamepad_pressed(_usbh_xinput, XINPUT_GAMEPAD_DPAD_RIGHT))
            _usbd_sbattalion->in.tunerDial += (_usbd_sbattalion->in.tunerDial < 15) ? 1 : -15;

        if (usbh_xinput_was_gamepad_pressed(_usbh_xinput, XINPUT_GAMEPAD_DPAD_DOWN) ||
            usbh_xinput_was_gamepad_pressed(_usbh_xinput, XINPUT_GAMEPAD_DPAD_LEFT))
            _usbd_sbattalion->in.tunerDial -= (_usbd_sbattalion->in.tunerDial > 0) ? 1 : -15;
    }
    //The default configuration
    else if (!usbh_xinput_is_chatpad_pressed(_usbh_xinput, XINPUT_CHATPAD_ORANGE))
    {
        //Apply chatpad alt2 direct mappings
        for (uint8_t i = 0; i < (sizeof(sb_chatpad_alt2_map) / sizeof(sb_chatpad_alt2_map[0])); i++)
        {
            if (usbh_xinput_is_chatpad_pressed(_usbh_xinput, pgm_read_word(&sb_chatpad_alt2_map[i].xinput_mask)))
                _usbd_sbattalion->in.wButtons[pgm_read_byte(&sb_chatpad_alt2_map[i].sb_word_offset)] |=
                    pgm_read_word(&sb_chatpad_alt2_map[i].sb_mask);
        }

        //Change gears by Pressing DUP or DDOWN.
        //To prevent accidentally changing gears when rotating, I check to make sure you aren't pressing LEFT or RIGHT.
        if (!(usbh_xstate->wButtons & XINPUT_GAMEPAD_DPAD_LEFT) &&
            !(usbh_xstate->wButtons & XINPUT_GAMEPAD_DPAD_RIGHT))
        {
            if (usbh_xinput_was_gamepad_pressed(_usbh_xinput, XINPUT_GAMEPAD_DPAD_UP))
                _usbd_sbattalion->in.gearLever += (_usbd_sbattalion->in.gearLever < SBC_GEAR_5) ? 1 : 0;
            if (usbh_xinput_was_gamepad_pressed(_usbh_xinput, XINPUT_GAMEPAD_DPAD_DOWN))
                _usbd_sbattalion->in.gearLever -= (_usbd_sbattalion->in.gearLever > SBC_GEAR_R) ? 1 : 0;
        }
    }

    //Shift will turn all switches off or on.
    if (usbh_xinput_was_chatpad_pressed(_usbh_xinput, XINPUT_CHATPAD_SHIFT))
        (_usbd_sbattalion->in.wButtons[2] &= 0xFFFC) ? _usbd_sbattalion->in.wButtons[2] &= ~0xFFFC :
                                                       _usbd_sbattalion->in.wButtons[2] |= 0xFFFC;

    //Apply Pedals
    _usbd_sbattalion->in.leftPedal     = (uint16_t)(usbh_xstate->bLeftTrigger << 8);
    _usbd_sbattalion->in.rightPedal    = (uint16_t)(usbh_xstate->bRightTrigger << 8);
    _usbd_sbattalion->in.middlePedal   = usbh_xinput_is_chatpad_pressed(_usbh_xinput, XINPUT_CHATPAD_BACK) ? 0xFF00 : 0x0000;
    _usbd_sbattalion->in.rotationLever = usbh_xinput_is_chatpad_pressed(_usbh_xinput, XINPUT_CHATPAD_MESSENGER) ? 0 :
                                         (usbh_xstate->wButtons & XINPUT_GAMEPAD_BACK) ? 0 :
                                         (usbh_xstate->wButtons & XINPUT_GAMEPAD_DPAD_LEFT) ? -32768 :
                                         (usbh_xstate->wButtons & XINPUT_GAMEPAD_DPAD_RIGHT) ? 32767 : 0;

    //Apply analog sticks
    _usbd_sbattalion->in.sightChangeX = _usbh_xinput->pad_state.sThumbLX;
    _usbd_sbattalion->in.sightChangeY = -_usbh_xinput->pad_state.sThumbLY - 1;

    //Moving aiming stick like a mouse cursor
    static const int16_t DEADZONE = 7500;
    static const int16_t sensitivity = 400;
    int32_t axisVal;
    axisVal = _usbh_xinput->pad_state.sThumbRX;
    if (abs(axisVal) > DEADZONE)
    {
        _user_data->vmouse_x += axisVal / sensitivity;
    }

    axisVal = _usbh_xinput->pad_state.sThumbRY;
    if (abs(axisVal) > DEADZONE)
    {
        _user_data->vmouse_y -= axisVal / sensitivity;
    }

    if (_user_data->vmouse_x < 0)          _user_data->vmouse_x = 0;
    if (_user_data->vmouse_x > UINT16_MAX) _user_data->vmouse_x = UINT16_MAX;
    if (_user_data->vmouse_y > UINT16_MAX) _user_data->vmouse_y = UINT16_MAX;
    if (_user_data->vmouse_y < 0)          _user_data->vmouse_y = 0;

    //Recentre the aiming stick if you hold the right stick in.
    if (usbh_xstate->wButtons & XINPUT_GAMEPAD_LEFT_THUMB)
    {
        if ((millis() - _user_data->button_hold_timer) > 500)
        {
            _user_data->vmouse_x = SBC_AIMING_MID;
            _user_data->vmouse_y = SBC_AIMING_MID;
        }
    }
    else
    {
        _user_data->button_hold_timer = millis();
    }

    _usbd_sbattalion->in.aimingX = (uint16_t)_user_data->vmouse_x;
    _usbd_sbattalion->in.aimingY = (uint16_t)_user_data->vmouse_y;

    //Apply rumble feedback back of LED feedback of critical buttons;
    _usbh_xinput->lValue_requested = _usbd_sbattalion->out.Chaff_Extinguisher;
    _usbh_xinput->lValue_requested |= _usbd_sbattalion->out.Chaff_Extinguisher << 4;
    _usbh_xinput->lValue_requested |= _usbd_sbattalion->out.Comm1_MagazineChange << 4;
    _usbh_xinput->lValue_requested |= _usbd_sbattalion->out.CockpitHatch_EmergencyEject << 4;
    _usbh_xinput->rValue_requested = _usbh_xinput->lValue_requested;

    //Hack: Cannot have SBC_W0_COCKPITHATCH and SBC_W0_IGNITION or aiming stick non zeros at the same time
    //or we trigger an IGR or shutdown with some Scene Bioses.
    if (_usbd_sbattalion->in.wButtons[0] & SBC_W0_IGNITION)
    {
        _usbd_sbattalion->in.aimingX = 0;
        _usbd_sbattalion->in.aimingY = 0;
        _usbd_sbattalion->in.wButtons[0] &= ~SBC_W0_COCKPITHATCH;
    }
}
