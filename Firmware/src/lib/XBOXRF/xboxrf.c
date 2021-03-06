#include <Arduino.h>

int8_t cmd_boot[10] = {0, 0, 1, 0, 0, 0, 0, 1, 0, 1};     //Init and show boot anim
int8_t cmd_sync[10] = {0, 0, 0, 0, 0, 0, 0, 1, 0, 0};     //Sync
int8_t cmd_greenled[10] = {0, 0, 1, 0, 1, 1, 0, 0, 0, 0}; //Set green LEDs, Lowest 4 bits controller quadrant
int8_t cmd_redled[10] = {0, 0, 1, 0, 1, 0, 0, 0, 0, 0};   //Set red LEDs, Lowest 4 bits controller quadrant

static int8_t _data_pin = -1;
static int8_t _clock_pin = -1;
static int8_t _connected[4] = {0, 0, 0, 0};

static void rf360_send(int8_t *cmd)
{
    if (_data_pin == -1)
        return;

    if (_clock_pin == -1)
        return;

    pinMode(_data_pin, OUTPUT);
    digitalWrite(_data_pin, LOW);
    //Bitbang serial out
    int prev = 1;
    for (int i = 0; i < 10; i++)
    {
        digitalWrite(LED_BUILTIN, HIGH);
        while (prev == digitalRead(_clock_pin))
            ;
        prev = digitalRead(_clock_pin);
        digitalWrite(_data_pin, cmd[i]);
        while (prev == digitalRead(_clock_pin))
            ;
        prev = digitalRead(_clock_pin);
        digitalWrite(LED_BUILTIN, LOW);
    }
    digitalWrite(_data_pin, HIGH);
    pinMode(_data_pin, INPUT);
}

void rf360_init(int data_pin, int clock_pin)
{
    _data_pin = data_pin;
    _clock_pin = clock_pin;
    pinMode(_data_pin, INPUT);
    pinMode(_clock_pin, INPUT);
    //Required on boot
    rf360_send(cmd_boot);
    //Send sync commannd every boot (FIXME?)
    rf360_send(cmd_sync);
}

void rf360_updateled(int controller, int8_t connected)
{
    if (controller >= 4)
        return;

    //If the connection state of the controller has changed,
    //update that LED quadrant accordingly
    if (connected != _connected[controller])
    {
        int8_t cmd[10];
        memcpy(cmd, cmd_greenled, sizeof(cmd));
        cmd[6 + controller] = (int8_t)connected;
        rf360_send(cmd);
        _connected[controller] = (connected > 0);
    }
}