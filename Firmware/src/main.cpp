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

#include "settings.h"
#include "xiddevice.h"
#include "Wire.h"
#include "EEPROM.h"

#ifdef MASTER
#include <XBOXRECV.h>
#include <usbhub.h>
#ifdef SUPPORTWIREDXBOXONE
#include <XBOXONE.h>
#endif
#ifdef SUPPORTWIREDXBOX360
#include <XBOXUSB.h>
#endif
#endif

//playerID is set in the main program based on the slot the Arduino is installed.
uint8_t playerID;
//Xbox gamepad data structure to store all button and actuator states for all four controllers.
USB_XboxGamepad_Data_t XboxOGDuke[MAX_CONTROLLERS];
//Default XID device to emulate
uint8_t ConnectedXID = DUKE_CONTROLLER;
//Flag is set when the device has been successfully setup by the OG Xbox
bool enumerationComplete = false;
//Timer used to time disconnection between SB and Duke controller swapover
uint32_t disconnectTimer = 0;

#ifdef SUPPORTBATTALION

USB_XboxSteelBattalion_Data_t XboxOGSteelBattalion;
USB_XboxSteelBattalion_Feedback_t XboxOGSteelBattalionFeedback;
#endif

#ifdef MASTER
USB UsbHost;
USBHub Hub(&UsbHost);
XBOXRECV Xbox360Wireless(&UsbHost);
uint8_t getButtonPress(ButtonEnum b, uint8_t controller);
int16_t getAnalogHat(AnalogHatEnum a, uint8_t controller);
void setRumbleOn(uint8_t lValue, uint8_t rValue, uint8_t controller);
void setLedOn(LEDEnum led, uint8_t controller);
bool controllerConnected(uint8_t controller);
#ifdef SUPPORTWIREDXBOXONE
XBOXONE XboxOneWired1(&UsbHost);
XBOXONE XboxOneWired2(&UsbHost);
XBOXONE XboxOneWired3(&UsbHost);
XBOXONE XboxOneWired4(&UsbHost);
XBOXONE *XboxOneWired[4] = {&XboxOneWired1, &XboxOneWired2, &XboxOneWired3, &XboxOneWired4};
#endif
#ifdef SUPPORTWIREDXBOX360
XBOXUSB Xbox360Wired1(&UsbHost);
XBOXUSB Xbox360Wired2(&UsbHost);
XBOXUSB Xbox360Wired3(&UsbHost);
XBOXUSB Xbox360Wired4(&UsbHost);
XBOXUSB *Xbox360Wired[4] = {&Xbox360Wired1, &Xbox360Wired2, &Xbox360Wired3, &Xbox360Wired4};
#endif
#endif

/*** Slave I2C Requests ***/
#ifndef MASTER
uint8_t inputBuffer[50]; //Input buffer used by slave devices
//This function executes whenever a data request is sent from the I2C Master.
//The master only requests the actuator values from the slave.
void sendRumble()
{
    Wire.write(&XboxOGDuke[0].left_actuator, 1);
    Wire.write(&XboxOGDuke[0].right_actuator, 1);
}

//This function executes whenever data is sent from the I2C Master.
//The master sends either the controller state if a wireless controller
//is synced or a disable packet {0xF0} if a controller is not synced.
void getControllerData(int len)
{
    for (int i = 0; i < len; i++)
    {
        inputBuffer[i] = Wire.read();
    }
    //0xF0 is a packet sent from the master if the respective wireless controller isn't synced.
    if (inputBuffer[0] == 0xF0)
    {
        USB_Detach();
        digitalWrite(ARDUINO_LED_PIN, HIGH);
    }
    //0xAA is a ping to see if the slave module is connected
    //Flash the LED to confirm receipt.
    else if (inputBuffer[0] == 0xAA)
    {
        digitalWrite(ARDUINO_LED_PIN, LOW);
        delay(250);
        digitalWrite(ARDUINO_LED_PIN, HIGH);
    }
    else
    {
        USB_Attach();
        if (enumerationComplete)
            digitalWrite(ARDUINO_LED_PIN, LOW);
    }
}
#endif
/*** End Slave I2C Requests ***/

int main(void)
{
    //Init the Arduino Library
    init();

    //Init IO
    pinMode(USB_HOST_RESET_PIN, OUTPUT);
    pinMode(ARDUINO_LED_PIN, OUTPUT);
    pinMode(PLAYER_ID1_PIN, INPUT_PULLUP);
    pinMode(PLAYER_ID2_PIN, INPUT_PULLUP);
    digitalWrite(USB_HOST_RESET_PIN, LOW);
    digitalWrite(ARDUINO_LED_PIN, HIGH);

    //Init the LUFA USB Device Library
    SetupHardware();
    GlobalInterruptEnable();

    //Initialise the Serial Port
    //Serial1.begin(500000);

    //Determine what player this board is. Used for the slave devices mainly.
    //There is 2 ID pins on the PCB which are read in.
    //00 = Player 1
    //01 = Player 2
    //10 = Player 3
    //11 = Player 4
    playerID = digitalRead(PLAYER_ID1_PIN) << 1 | digitalRead(PLAYER_ID2_PIN);

    //Init the XboxOG data arrays to zero.
    memset(&XboxOGDuke, 0x00, sizeof(USB_XboxGamepad_Data_t) * MAX_CONTROLLERS);

/* MASTER DEVICE USB HOST CONTROLLER INIT */
#ifdef MASTER

    //Init Usb Host Controller
    digitalWrite(USB_HOST_RESET_PIN, LOW);
    delay(20); //wait 20ms to reset the IC. Reseting at startup improves reliability in my experience.
    digitalWrite(USB_HOST_RESET_PIN, HIGH);
    delay(20); //Settle
    while (UsbHost.Init() == -1)
    {
        digitalWrite(ARDUINO_LED_PIN, !digitalRead(ARDUINO_LED_PIN));
        delay(500);
    }

    //Init I2C Master
    Wire.begin();
    Wire.setClock(400000);

    //Ping slave devices if present
    //This will cause them to blink
    for (uint8_t i = 1; i < MAX_CONTROLLERS; i++)
    {
        static const char ping[] = {(char)0xAA};
        Wire.beginTransmission(i);
        Wire.write(ping, 1);
        Wire.endTransmission(true);
        delay(100);
    }

    //Init all chatpad led FIFO queues 0xFF means empty spot.
    for (uint8_t i = 0; i < 4; i++)
    {
        Xbox360Wireless.chatPadLedQueue[i][0] = 0xFF;
        Xbox360Wireless.chatPadLedQueue[i][1] = 0xFF;
        Xbox360Wireless.chatPadLedQueue[i][2] = 0xFF;
        Xbox360Wireless.chatPadLedQueue[i][3] = 0xFF;
        Xbox360Wireless.chatPadInitNeeded[i] = 1;
    }
#endif

#ifdef SUPPORTBATTALION
    //Setup EEPROM for variable sensitivity adjustment for SB controller.
    static int32_t sensitivity = 400;
    if (EEPROM.read(0x20) != 0xAB)
    {
        //Address 0x20 should contain 0xAB if it is initialized properly. If not, write it then set a default value below:
        EEPROM.write(0x20, 0xAB);
        //First time setup default EEPROM register. Address 0x00 contains the sensitivity value.
        EEPROM.put(0x00, sensitivity);
    }
#endif

/* END MASTER DEVICE USB HOST CONTROLLER INIT */

/* SLAVE I2C SLAVE INIT */
#ifndef MASTER
    //Init I2C Slave
    Wire.begin(playerID); //I2C Address is 0x01,0x02,0x03 for Player 2,3 and 4 respectively.
    Wire.setClock(400000);
    Wire.onRequest(sendRumble);        //Register event for sendRumble. The host only requests actuator values.
    Wire.onReceive(getControllerData); //Register receive event for getting Xbox360 controller state data.
    Serial1.print(F("\r\nThis is a slave device"));
#endif
    /* END SLAVE I2C SLAVE INIT */

    while (1)
    {
#ifdef MASTER
        /*** MASTER TASKS ***/
        UsbHost.busprobe();

        for (uint8_t i = 0; i < MAX_CONTROLLERS; i++)
        {
            UsbHost.Task();
            if (controllerConnected(i))
            {
                //Button Mapping for Duke Controller
                if (ConnectedXID == DUKE_CONTROLLER || i != 0)
                {

                    //Read Digital Buttons
                    XboxOGDuke[i].dButtons=0x0000;
                    if (getButtonPress(UP, i))      XboxOGDuke[i].dButtons |= DUP;
                    if (getButtonPress(DOWN, i))    XboxOGDuke[i].dButtons |= DDOWN;
                    if (getButtonPress(LEFT, i))    XboxOGDuke[i].dButtons |= DLEFT;
                    if (getButtonPress(RIGHT, i))   XboxOGDuke[i].dButtons |= DRIGHT;;
                    if (getButtonPress(START, i))   XboxOGDuke[i].dButtons |= START_BTN;
                    if (getButtonPress(BACK, i))    XboxOGDuke[i].dButtons |= BACK_BTN;
                    if (getButtonPress(L3, i))      XboxOGDuke[i].dButtons |= LS_BTN;
                    if (getButtonPress(R3, i))      XboxOGDuke[i].dButtons |= RS_BTN;

                    //Read Analog Buttons - have to be converted to digital because x360 controllers don't have analog buttons
                    getButtonPress(A, i)    ? XboxOGDuke[i].A = 0xFF      : XboxOGDuke[i].A = 0x00;
                    getButtonPress(B, i)    ? XboxOGDuke[i].B = 0xFF      : XboxOGDuke[i].B = 0x00;
                    getButtonPress(X, i)    ? XboxOGDuke[i].X = 0xFF      : XboxOGDuke[i].X = 0x00;
                    getButtonPress(Y, i)    ? XboxOGDuke[i].Y = 0xFF      : XboxOGDuke[i].Y = 0x00;
                    getButtonPress(L1, i)   ? XboxOGDuke[i].WHITE = 0xFF  : XboxOGDuke[i].WHITE = 0x00;
                    getButtonPress(R1, i)   ? XboxOGDuke[i].BLACK = 0xFF  : XboxOGDuke[i].BLACK = 0x00;

                    //Read Analog triggers
                    XboxOGDuke[i].L = getButtonPress(L2, i); //0x00 to 0xFF
                    XboxOGDuke[i].R = getButtonPress(R2, i); //0x00 to 0xFF

                    //Read Control Sticks (16bit signed short)
                    XboxOGDuke[i].leftStickX = getAnalogHat(LeftHatX, i);
                    XboxOGDuke[i].leftStickY = getAnalogHat(LeftHatY, i);
                    XboxOGDuke[i].rightStickX = getAnalogHat(RightHatX, i);
                    XboxOGDuke[i].rightStickY = getAnalogHat(RightHatY, i);
                }
#ifdef SUPPORTBATTALION
                //Button Mapping for Steel Battalion Controller - only applicable for player 1 and Xbox 360 Wireless Controllers
                else if (ConnectedXID == STEELBATTALION && Xbox360Wireless.Xbox360Connected[i] && i == 0)
                {
                    //R,N,1,2,3,4,5
                    static const uint8_t gearStates[7] = {7, 8, 9, 10, 11, 12, 13}; 
                    static int8_t currentGear = 1;                                  
                    static int32_t virtualMouseX = 32768, virtualMouseY = 32768;

                    XboxOGSteelBattalion.dButtons[0] = 0x0000;
                    XboxOGSteelBattalion.dButtons[1] = 0x0000;
                    XboxOGSteelBattalion.dButtons[2] &= 0xFFFC; //Need to only clear the two LSBs. The other bits are the toggle switches

                    //L1/R1 = L and R bumpers
                    //L2/R2 = L and R Triggers
                    //L3/R3 - L and R Stick Press
                    //Note the W0,W1 or W2 in the SBC_GAMEPAD button defines the offset in dButtons[X].
                    //i.e. SBC_GAMEPAD_W1_COMM3 should use dButtons[1].
                    if (Xbox360Wireless.getButtonPress(START, i))
                        XboxOGSteelBattalion.dButtons[0] |= SBC_GAMEPAD_W0_START;
                    if (Xbox360Wireless.getButtonPress(L1, i))
                        XboxOGSteelBattalion.dButtons[0] |= SBC_GAMEPAD_W0_RIGHTJOYFIRE;
                    static uint32_t L3HoldTimer = 0; //Timer for holding the Left stick in
                    if (Xbox360Wireless.getButtonPress(L3, i))
                    {
                        XboxOGSteelBattalion.dButtons[2] |= SBC_GAMEPAD_W2_LEFTJOYSIGHTCHANGE;
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

                    if (Xbox360Wireless.getButtonPress(R3, i) || Xbox360Wireless.getButtonPress(B, i))
                        XboxOGSteelBattalion.dButtons[0] |= SBC_GAMEPAD_W0_RIGHTJOYLOCKON;

                    if (Xbox360Wireless.getButtonPress(R1, i) || Xbox360Wireless.getButtonPress(A, i))
                        XboxOGSteelBattalion.dButtons[0] |= SBC_GAMEPAD_W0_RIGHTJOYMAINWEAPON;

                    if (Xbox360Wireless.getButtonPress(XBOX, i) || Xbox360Wireless.getChatPadPress(CHATPAD_0, i))
                        XboxOGSteelBattalion.dButtons[0] |= SBC_GAMEPAD_W0_EJECT;

                    //What the X button does depends on what is needed by your VT.
                    //It will Extinguish, Reload (if empty), or Wash if required. It will rumble for Chaff but you need to press Y to chaff.
                    //This is determined by reading back the LED feedback from the console. The game normally
                    //makes these LEDs flash when action is required. I use this data to rumble the controller and determine what X should do.
                    if ((XboxOGSteelBattalionFeedback.Chaff_Extinguisher & 0xF0) != 0)
                    {
                        XboxOGDuke[i].right_actuator = (XboxOGSteelBattalionFeedback.Chaff_Extinguisher << 0) & 0xF0; //Only use right motor for chaff
                        XboxOGDuke[i].rumbleUpdate = 1;
                    }
                    if ((XboxOGSteelBattalionFeedback.Chaff_Extinguisher & 0x0F) != 0)
                    {
                        XboxOGDuke[i].left_actuator = (XboxOGSteelBattalionFeedback.Chaff_Extinguisher << 4) & 0xF0;
                        XboxOGDuke[i].right_actuator = XboxOGDuke[i].left_actuator;
                        XboxOGDuke[i].rumbleUpdate = 1;
                        if (Xbox360Wireless.getButtonPress(X, i))
                            XboxOGSteelBattalion.dButtons[1] |= SBC_GAMEPAD_W1_EXTINGUISHER;
                    }
                    if ((XboxOGSteelBattalionFeedback.Comm1_MagazineChange & 0x0F) != 0)
                    {
                        XboxOGDuke[i].left_actuator = (XboxOGSteelBattalionFeedback.Comm1_MagazineChange << 4) & 0xF0;
                        XboxOGDuke[i].right_actuator = XboxOGDuke[i].left_actuator;
                        XboxOGDuke[i].rumbleUpdate = 1;
                        if (Xbox360Wireless.getButtonPress(X, i))
                            XboxOGSteelBattalion.dButtons[1] |= SBC_GAMEPAD_W1_WEAPONCONMAGAZINE;
                    }
                    if ((XboxOGSteelBattalionFeedback.Washing_LineColorChange & 0xF0) != 0)
                    {
                        if (Xbox360Wireless.getButtonPress(X, i))
                            XboxOGSteelBattalion.dButtons[1] |= SBC_GAMEPAD_W1_WASHING;
                    }
                    if ((XboxOGSteelBattalionFeedback.CockpitHatch_EmergencyEject & 0x0F) != 0)
                    {
                        XboxOGDuke[i].left_actuator = (XboxOGSteelBattalionFeedback.CockpitHatch_EmergencyEject << 4) & 0xF0;
                        XboxOGDuke[i].right_actuator = XboxOGDuke[i].left_actuator;
                        XboxOGDuke[i].rumbleUpdate = 1;
                    }

                    //Hold the messenger button for COMMS and Adjust TunerDial
                    if (Xbox360Wireless.getChatPadPress(CHATPAD_MESSENGER, i) || Xbox360Wireless.getButtonPress(BACK, i))
                    {
                        if (Xbox360Wireless.getChatPadPress(CHATPAD_1, i))
                            XboxOGSteelBattalion.dButtons[1] |= SBC_GAMEPAD_W1_COMM1;
                        if (Xbox360Wireless.getChatPadPress(CHATPAD_2, i))
                            XboxOGSteelBattalion.dButtons[1] |= SBC_GAMEPAD_W1_COMM2;
                        if (Xbox360Wireless.getChatPadPress(CHATPAD_3, i))
                            XboxOGSteelBattalion.dButtons[1] |= SBC_GAMEPAD_W1_COMM3;
                        if (Xbox360Wireless.getChatPadPress(CHATPAD_4, i))
                            XboxOGSteelBattalion.dButtons[1] |= SBC_GAMEPAD_W1_COMM4;
                        if (Xbox360Wireless.getChatPadPress(CHATPAD_5, i))
                            XboxOGSteelBattalion.dButtons[2] |= SBC_GAMEPAD_W2_COMM5;

                        //Change tuner dial position by Holding the messenger then pressing D-pad directions.
                        //Tuner dial = 0-15, corresponding to the 9o'clock position going clockwise.
                        if (Xbox360Wireless.getButtonClick(UP, i) || Xbox360Wireless.getButtonClick(RIGHT, i))
                            XboxOGSteelBattalion.tunerDial += 2;

                        if (Xbox360Wireless.getButtonClick(DOWN, i) || Xbox360Wireless.getButtonClick(LEFT, i))
                            XboxOGSteelBattalion.tunerDial -= 2;

                        if (XboxOGSteelBattalion.tunerDial > 15)
                            XboxOGSteelBattalion.tunerDial = 15;
                        if (XboxOGSteelBattalion.tunerDial < 0)
                            XboxOGSteelBattalion.tunerDial = 0;

                        //The default configuration
                    }
                    else if (!Xbox360Wireless.getChatPadPress(CHATPAD_ORANGE, i))
                    {
                        if (Xbox360Wireless.getChatPadPress(CHATPAD_1, i))
                            XboxOGSteelBattalion.dButtons[1] |= SBC_GAMEPAD_W1_FUNCTIONF1;
                        if (Xbox360Wireless.getChatPadPress(CHATPAD_2, i))
                            XboxOGSteelBattalion.dButtons[1] |= SBC_GAMEPAD_W1_FUNCTIONTANKDETACH;
                        if (Xbox360Wireless.getChatPadPress(CHATPAD_3, i))
                            XboxOGSteelBattalion.dButtons[0] |= SBC_GAMEPAD_W0_FUNCTIONFSS;
                        if (Xbox360Wireless.getChatPadPress(CHATPAD_4, i))
                            XboxOGSteelBattalion.dButtons[1] |= SBC_GAMEPAD_W1_FUNCTIONF2;
                        if (Xbox360Wireless.getChatPadPress(CHATPAD_5, i))
                            XboxOGSteelBattalion.dButtons[1] |= SBC_GAMEPAD_W1_FUNCTIONOVERRIDE;
                        if (Xbox360Wireless.getChatPadPress(CHATPAD_6, i))
                            XboxOGSteelBattalion.dButtons[0] |= SBC_GAMEPAD_W0_FUNCTIONMANIPULATOR;
                        if (Xbox360Wireless.getChatPadPress(CHATPAD_7, i))
                            XboxOGSteelBattalion.dButtons[1] |= SBC_GAMEPAD_W1_FUNCTIONF3;
                        if (Xbox360Wireless.getChatPadPress(CHATPAD_8, i))
                            XboxOGSteelBattalion.dButtons[1] |= SBC_GAMEPAD_W1_FUNCTIONNIGHTSCOPE;
                        if (Xbox360Wireless.getChatPadPress(CHATPAD_9, i))
                            XboxOGSteelBattalion.dButtons[0] |= SBC_GAMEPAD_W0_FUNCTIONLINECOLORCHANGE;

                        //Change gears by Pressing DUP or DDOWN. Limits are 0-6. //R,N,1,2,3,4,5
                        //To prevent accidentally changing gears whilst rotating, I check to make sure you aren't pressing LEFT or RIGHT.
                        if (Xbox360Wireless.getButtonClick(UP, i) && !(Xbox360Wireless.getButtonPress(LEFT, i) || Xbox360Wireless.getButtonPress(RIGHT, i)))
                        {
                            currentGear++;
                        }
                        else if (Xbox360Wireless.getButtonClick(DOWN, i) && !(Xbox360Wireless.getButtonPress(LEFT, i) || Xbox360Wireless.getButtonPress(RIGHT, i)))
                        {
                            currentGear--;
                        }
                        if (currentGear > 6)
                            currentGear = 6;
                        if (currentGear < 0)
                            currentGear = 0;
                        XboxOGSteelBattalion.gearLever = gearStates[currentGear];
                    }

                    if (Xbox360Wireless.getChatPadClick(CHATPAD_Q, i))
                        XboxOGSteelBattalion.dButtons[2] ^= SBC_GAMEPAD_W2_TOGGLEOXYGENSUPPLY; //Toggle Switch
                    if (Xbox360Wireless.getChatPadClick(CHATPAD_A, i))
                        XboxOGSteelBattalion.dButtons[2] ^= SBC_GAMEPAD_W2_TOGGLEFILTERCONTROL; //Toggle Switch
                    if (Xbox360Wireless.getChatPadClick(CHATPAD_W, i))
                        XboxOGSteelBattalion.dButtons[2] ^= SBC_GAMEPAD_W2_TOGGLEVTLOCATION; //Toggle Switch
                    if (Xbox360Wireless.getChatPadClick(CHATPAD_S, i))
                        XboxOGSteelBattalion.dButtons[2] ^= SBC_GAMEPAD_W2_TOGGLEBUFFREMATERIAL; //Toggle Switch
                    if (Xbox360Wireless.getChatPadClick(CHATPAD_Z, i))
                        XboxOGSteelBattalion.dButtons[2] ^= SBC_GAMEPAD_W2_TOGGLEFUELFLOWRATE; //Toggle Switch
                    if (Xbox360Wireless.getChatPadPress(CHATPAD_D, i))
                        XboxOGSteelBattalion.dButtons[1] |= SBC_GAMEPAD_W1_WASHING;
                    if (Xbox360Wireless.getChatPadPress(CHATPAD_F, i))
                        XboxOGSteelBattalion.dButtons[1] |= SBC_GAMEPAD_W1_EXTINGUISHER;
                    if (Xbox360Wireless.getChatPadClick(CHATPAD_SHIFT, i))
                    {
                        if (XboxOGSteelBattalion.dButtons[2] &= 0xFFFC)
                        { //If any of the toggle switches are on SHIFT will turn everything off.
                            XboxOGSteelBattalion.dButtons[2] &= ~0xFFFC;
                        }
                        else
                        {
                            XboxOGSteelBattalion.dButtons[2] |= 0xFFFC; //If all toggle switches are OFF, this will quickly turn them all on
                        }
                    }

                    if (Xbox360Wireless.getChatPadPress(CHATPAD_G, i) || Xbox360Wireless.getButtonPress(Y, i))
                        XboxOGSteelBattalion.dButtons[1] |= SBC_GAMEPAD_W1_CHAFF;

                    if (Xbox360Wireless.getChatPadPress(CHATPAD_X, i) || Xbox360Wireless.getChatPadPress(CHATPAD_RIGHT, i))
                        XboxOGSteelBattalion.dButtons[1] |= SBC_GAMEPAD_W1_WEAPONCONMAIN;

                    if (Xbox360Wireless.getChatPadPress(CHATPAD_C, i) || Xbox360Wireless.getChatPadPress(CHATPAD_LEFT, i))
                        XboxOGSteelBattalion.dButtons[1] |= SBC_GAMEPAD_W1_WEAPONCONSUB;

                    if (Xbox360Wireless.getChatPadPress(CHATPAD_V, i) || Xbox360Wireless.getChatPadPress(CHATPAD_SPACE, i))
                        XboxOGSteelBattalion.dButtons[1] |= SBC_GAMEPAD_W1_WEAPONCONMAGAZINE;

                    if (Xbox360Wireless.getChatPadPress(CHATPAD_U, i))
                        XboxOGSteelBattalion.dButtons[0] |= SBC_GAMEPAD_W0_MULTIMONOPENCLOSE;
                    if (Xbox360Wireless.getChatPadPress(CHATPAD_J, i))
                        XboxOGSteelBattalion.dButtons[0] |= SBC_GAMEPAD_W0_MULTIMONMODESELECT;
                    if (Xbox360Wireless.getChatPadPress(CHATPAD_N, i))
                        XboxOGSteelBattalion.dButtons[0] |= SBC_GAMEPAD_W0_MAINMONZOOMIN;
                    if (Xbox360Wireless.getChatPadPress(CHATPAD_I, i))
                        XboxOGSteelBattalion.dButtons[0] |= SBC_GAMEPAD_W0_MULTIMONMAPZOOMINOUT;
                    if (Xbox360Wireless.getChatPadPress(CHATPAD_K, i))
                        XboxOGSteelBattalion.dButtons[0] |= SBC_GAMEPAD_W0_MULTIMONSUBMONITOR;
                    if (Xbox360Wireless.getChatPadPress(CHATPAD_M, i))
                        XboxOGSteelBattalion.dButtons[0] |= SBC_GAMEPAD_W0_MAINMONZOOMOUT;
                    if (Xbox360Wireless.getChatPadPress(CHATPAD_ENTER, i))
                        XboxOGSteelBattalion.dButtons[0] |= SBC_GAMEPAD_W0_START;

                    if (Xbox360Wireless.getChatPadPress(CHATPAD_P, i))
                    {
                        XboxOGSteelBattalion.dButtons[0] |= SBC_GAMEPAD_W0_COCKPITHATCH;
                        XboxOGSteelBattalion.dButtons[0] &= ~SBC_GAMEPAD_W0_IGNITION; //Cannot have these two buttons pressed at the same time, some bioses will trigger an IGR
                    }

                    if (Xbox360Wireless.getChatPadPress(CHATPAD_COMMA, i))
                    {
                        XboxOGSteelBattalion.dButtons[0] |= SBC_GAMEPAD_W0_IGNITION;
                        XboxOGSteelBattalion.dButtons[0] &= ~SBC_GAMEPAD_W0_COCKPITHATCH; //Cannot have these two buttons pressed at the same time, some bioses will trigger an IGR
                    }

                    /* Read Steel Battalion OUT endpoint for LED feedback from HOST to Device, this is not a standard HID Set Report, so is read here manually */
                    uint8_t ep = Endpoint_GetCurrentEndpoint();
                    Endpoint_SelectEndpoint(0x01); //0x01 is the out endpoint address for the SB Controller
                    if (Endpoint_IsOUTReceived())
                    {
                        Endpoint_Read_Stream_LE(&XboxOGSteelBattalionFeedback, 22, NULL);
                        Endpoint_ClearOUT();
                    }
                    Endpoint_SelectEndpoint(ep); //set back to the old endpoint.

                    //Apply Pedals
                    XboxOGSteelBattalion.leftPedal = (uint16_t)(Xbox360Wireless.getButtonPress(L2, i) << 8);  //0x00 to 0xFF00 SIDESTEP PEDAL
                    XboxOGSteelBattalion.rightPedal = (uint16_t)(Xbox360Wireless.getButtonPress(R2, i) << 8); //0x00 to 0xFF00 ACCEL PEDAL
                    if (Xbox360Wireless.getChatPadPress(CHATPAD_BACK, i))
                    {
                        XboxOGSteelBattalion.middlePedal = 0xFF00; //Brake Pedal
                    }
                    else
                    {
                        XboxOGSteelBattalion.middlePedal = 0x0000; //Brake Pedal
                    }

                    if (!Xbox360Wireless.getChatPadPress(CHATPAD_MESSENGER, i) && !Xbox360Wireless.getButtonPress(BACK, i))
                    {
                        if (Xbox360Wireless.getButtonPress(LEFT, i))
                        {
                            XboxOGSteelBattalion.rotationLever = -32767;
                        }
                        else if (Xbox360Wireless.getButtonPress(RIGHT, i))
                        {
                            XboxOGSteelBattalion.rotationLever = +32767;
                        }
                        else
                        {
                            XboxOGSteelBattalion.rotationLever = 0;
                        }
                    }

                    //Apply analog sticks
                    static int32_t sensitivity = 400;
                    EEPROM.get(0x00, sensitivity);
                    if (Xbox360Wireless.getChatPadPress(CHATPAD_ORANGE, i))
                    {
                        if (Xbox360Wireless.getChatPadPress(CHATPAD_9, i))
                            sensitivity = 200;
                        if (Xbox360Wireless.getChatPadPress(CHATPAD_8, i))
                            sensitivity = 250;
                        if (Xbox360Wireless.getChatPadPress(CHATPAD_7, i))
                            sensitivity = 300;
                        if (Xbox360Wireless.getChatPadPress(CHATPAD_6, i))
                            sensitivity = 350;
                        if (Xbox360Wireless.getChatPadPress(CHATPAD_5, i))
                            sensitivity = 400;
                        if (Xbox360Wireless.getChatPadPress(CHATPAD_4, i))
                            sensitivity = 650;
                        if (Xbox360Wireless.getChatPadPress(CHATPAD_3, i))
                            sensitivity = 800;
                        if (Xbox360Wireless.getChatPadPress(CHATPAD_2, i))
                            sensitivity = 1000;
                        if (Xbox360Wireless.getChatPadPress(CHATPAD_1, i))
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

                    XboxOGSteelBattalion.sightChangeX = Xbox360Wireless.getAnalogHat(LeftHatX, i);
                    XboxOGSteelBattalion.sightChangeY = -Xbox360Wireless.getAnalogHat(LeftHatY, i) - 1;

                    if (!Xbox360Wireless.getChatPadPress(CHATPAD_MESSENGER, i) && !Xbox360Wireless.getButtonPress(BACK, i))
                    {
                        //Moving aiming stick like a mouse cursor

                        int32_t axisVal = Xbox360Wireless.getAnalogHat(RightHatX, i);
                        if (axisVal > 7500 || axisVal < -7500)
                        {
                            virtualMouseX += axisVal / sensitivity;
                        }

                        axisVal = Xbox360Wireless.getAnalogHat(RightHatY, i);
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

                        XboxOGSteelBattalion.aimingX = (uint16_t)virtualMouseX;
                        XboxOGSteelBattalion.aimingY = (uint16_t)virtualMouseY;
                    }

                    XboxOGSteelBattalion.sightChangeX = Xbox360Wireless.getAnalogHat(LeftHatX, i);
                    XboxOGSteelBattalion.sightChangeY = -Xbox360Wireless.getAnalogHat(LeftHatY, i) - 1;
                }

                //Press the GREEN & ORANGE button on the chatpad to toggle between Duke and the Steel Battalion.
                if (Xbox360Wireless.getChatPadPress(CHATPAD_GREEN, 0) && Xbox360Wireless.getChatPadClick(CHATPAD_ORANGE, 0))
                {
                    USB_Detach();
                    disconnectTimer = millis();
                    if (ConnectedXID != STEELBATTALION)
                    {
                        Xbox360Wireless.chatPadQueueLed(CHATPAD_LED_GREEN_OFF, i);
                        Xbox360Wireless.chatPadQueueLed(CHATPAD_LED_ORANGE_ON, i);
                        Xbox360Wireless.chatPadQueueLed(CHATPAD_LED_GREEN_OFF, i);
                        Xbox360Wireless.chatPadQueueLed(CHATPAD_LED_ORANGE_ON, i);
                    }
                    else
                    {
                        Xbox360Wireless.chatPadQueueLed(CHATPAD_LED_GREEN_ON, i);
                        Xbox360Wireless.chatPadQueueLed(CHATPAD_LED_ORANGE_OFF, i);
                        Xbox360Wireless.chatPadQueueLed(CHATPAD_LED_GREEN_ON, i);
                        Xbox360Wireless.chatPadQueueLed(CHATPAD_LED_ORANGE_OFF, i);
                    }
                }
                if (disconnectTimer != 0 && millis() - disconnectTimer > 500)
                {
                    if (ConnectedXID != STEELBATTALION)
                    {
                        ConnectedXID = STEELBATTALION;
                        XboxOGDuke[0].left_actuator = 0;
                        XboxOGDuke[0].right_actuator = 0;
                        XboxOGDuke[0].rumbleUpdate = 1;
                        XboxOGSteelBattalion.dButtons[0] = 0x0000;
                        XboxOGSteelBattalion.dButtons[1] = 0x0000;
                        XboxOGSteelBattalion.dButtons[2] = 0x0000;
                        Xbox360Wireless.chatPadQueueLed(CHATPAD_LED_GREEN_OFF, i);
                        Xbox360Wireless.chatPadQueueLed(CHATPAD_LED_ORANGE_ON, i);
                        Xbox360Wireless.chatPadQueueLed(CHATPAD_LED_GREEN_OFF, i);
                        Xbox360Wireless.chatPadQueueLed(CHATPAD_LED_ORANGE_ON, i);
                    }
                    else
                    {
                        ConnectedXID = DUKE_CONTROLLER;
                        XboxOGDuke[0].left_actuator = 0;
                        XboxOGDuke[0].right_actuator = 0;
                        XboxOGDuke[0].rumbleUpdate = 1;
                        XboxOGDuke[0].dButtons = 0x0000;
                        Xbox360Wireless.chatPadQueueLed(CHATPAD_LED_GREEN_ON, i);
                        Xbox360Wireless.chatPadQueueLed(CHATPAD_LED_ORANGE_OFF, i);
                        Xbox360Wireless.chatPadQueueLed(CHATPAD_LED_GREEN_ON, i);
                        Xbox360Wireless.chatPadQueueLed(CHATPAD_LED_ORANGE_OFF, i);
                    }
                    disconnectTimer = 0;
                }
#endif

                //Anything that sends a command to the Xbox 360 controllers happens here.
                //(i.e rumble, LED changes, controller off command)
                static uint32_t commandTimer[MAX_CONTROLLERS] = {0};
                static uint32_t xboxHoldTimer[MAX_CONTROLLERS] = {0};
                if (millis() - commandTimer[i] > 16)
                {
                    //If you hold the XBOX button for more than ~1second, turn off controller
                    if (getButtonPress(XBOX, i))
                    {
                        if (xboxHoldTimer[i] == 0)
                        {
                            xboxHoldTimer[i] = millis();
                        }
                        if ((millis() - xboxHoldTimer[i]) > 1000 && (millis() - xboxHoldTimer[i]) < 1100)
                        {
                            XboxOGDuke[i].dButtons = 0x00;
                            setRumbleOn(0, 0, i);
                            delay(10);
                            Xbox360Wireless.disconnect(i);
                            xboxHoldTimer[i] = 0;
                        }
                    }
                    //START+BACK TRIGGERS is a standard soft reset command.
                    //We turn off the rumble motors here to prevent them getting locked on
                    //if you happen to press this reset combo mid rumble.
                    else if (getButtonPress(START, i) && getButtonPress(BACK, i) &&
                             getButtonPress(L2, i) > 0x00 && getButtonPress(R2, i) > 0x00)
                    {
                        //Turn off rumble on all controllers
                        for (uint8_t j = 0; j < MAX_CONTROLLERS; j++)
                        {
                            XboxOGDuke[j].left_actuator = 0;
                            XboxOGDuke[j].right_actuator = 0;
                            XboxOGDuke[j].rumbleUpdate = 1;
                        }
                    }
                    //If Xbox button isnt held down, send the rumble commands
                    else
                    {
                        xboxHoldTimer[i] = 0; //Reset the XBOX button hold time counter.
                        if (XboxOGDuke[i].rumbleUpdate == 1)
                        {
                            setRumbleOn(XboxOGDuke[i].left_actuator, XboxOGDuke[i].right_actuator, i);
                            XboxOGDuke[i].rumbleUpdate = 0;
                        }
                    }
                    commandTimer[i] = millis();
                }

                //Send controller state to slave devices, and retrieve actuator/rumble values from slave devices.
                //Applicable to player 2, 3 and 4 only. i.e when i>0.
                static uint32_t rumblei2cTimer[MAX_CONTROLLERS] = {0}; //Timer to monitor how often rumbles are requested.
                if (i > 0)
                {
                    Wire.beginTransmission(i);
                    Wire.write((char *)&XboxOGDuke[i], 20);
                    Wire.endTransmission(true);
                    if (millis() - rumblei2cTimer[i] > 8)
                    {
                        if (Wire.requestFrom(i, (uint8_t)2) == 2)
                        {
                            int temp = Wire.read(); //read first 8 bytes - this is left actuator, returns -1 on error.
                            if (temp != -1 && XboxOGDuke[i].left_actuator != (uint8_t)temp)
                            {
                                XboxOGDuke[i].left_actuator = (uint8_t)temp;
                                XboxOGDuke[i].rumbleUpdate = 1;
                            }

                            temp = Wire.read(); //read second 8 bytes - this is right actuator, returns -1 on error.
                            if (temp != -1 && XboxOGDuke[i].right_actuator != (uint8_t)temp)
                            {
                                XboxOGDuke[i].right_actuator = (uint8_t)temp;
                                XboxOGDuke[i].rumbleUpdate = 1;
                            }
                        }
                        else
                        {
                            //just clear the buffer, must've been an error.
                            Wire.flush();
                        }
                        rumblei2cTimer[i] = millis();
                    }
                }

                /*Check/send the Player 1 HID report every loop to minimise lag even more on the master*/
                sendControllerHIDReport();
            }
            else
            {
                //If the respective controller isn't synced, we instead send a disablePacket over the i2c bus
                //so that the slave device knows to disable its USB. I've arbitrarily made this 0xF0.
                if (i > 0)
                {
                    static uint8_t disablePacket[1] = {0xF0};
                    Wire.beginTransmission(i);
                    Wire.write((char *)disablePacket, 1);
                    Wire.endTransmission(true);
                }
            }
        } //End master for loop

        //Handle Player 1 controller connect/disconnect events.
        if (controllerConnected(0) && disconnectTimer == 0)
        {
            USB_Attach();
            if (enumerationComplete)
            {
                digitalWrite(ARDUINO_LED_PIN, LOW);
            }
        }
        else if (millis() > 7000)
        {
            digitalWrite(ARDUINO_LED_PIN, HIGH);
            USB_Detach(); //Disconnect from the OG Xbox port.
            Xbox360Wireless.chatPadInitNeeded[0] = 1;
        }
        else
        {
            USB_Attach();
            sendControllerHIDReport();
        }

/***END MASTER TASKS ***/
#endif

        //THPS 2X is the only game I know that sends rumble commands to the USB OUT pipe
        //instead of the control pipe. So unfortunately need to manually read the out pipe
        //and update rumble values as needed!
        uint8_t ep = Endpoint_GetCurrentEndpoint();
        static uint8_t report[6];
        Endpoint_SelectEndpoint(0x02); //0x02 is the out endpoint address for the Duke Controller
        if (Endpoint_IsOUTReceived())
        {
            Endpoint_Read_Stream_LE(report, 6, NULL);
            Endpoint_ClearOUT();
            if (report[1] == 0x06)
            {
                XboxOGDuke[0].left_actuator = report[3];
                XboxOGDuke[0].right_actuator = report[5];
                XboxOGDuke[0].rumbleUpdate = 1;
            }
            report[1] = 0x00;
        }
        Endpoint_SelectEndpoint(ep); //set back to the old endpoint.

#ifndef MASTER
        if (inputBuffer[0] != 0xF0)
        {
            memcpy(&XboxOGDuke[0], inputBuffer, 20);
        }
        sendControllerHIDReport();
#endif
    }
}

/* Send the HID report to the OG Xbox */
void sendControllerHIDReport()
{
    switch (ConnectedXID)
    {
    case DUKE_CONTROLLER:
        if (USB_Device_GetFrameNumber() - DukeController_HID_Interface.State.PrevFrameNum >= 4)
        {
            HID_Device_USBTask(&DukeController_HID_Interface); //Send OG Xbox HID Report
        }
        break;
#ifdef SUPPORTBATTALION
    case STEELBATTALION:
        if (USB_Device_GetFrameNumber() - SteelBattalion_HID_Interface.State.PrevFrameNum >= 4)
        {
            HID_Device_USBTask(&SteelBattalion_HID_Interface); //Send OG Xbox HID Report
        }
        break;
#endif
    }
    USB_USBTask();
}

#ifdef MASTER
//Parse button presses for each type of controller
uint8_t getButtonPress(ButtonEnum b, uint8_t controller)
{
    if (Xbox360Wireless.Xbox360Connected[controller])
        return Xbox360Wireless.getButtonPress(b, controller);

#ifdef SUPPORTWIREDXBOX360
    if (Xbox360Wired[controller]->Xbox360Connected)
        return Xbox360Wired[controller]->getButtonPress(b);
#endif

#ifdef SUPPORTWIREDXBOXONE
    if (XboxOneWired[controller]->XboxOneConnected)
    {
        if (b == L2 || b == R2)
        {
            //Xbone one triggers are 10-bit, remove 2LSBs so its 8bit like OG Xbox
            return (uint8_t)(XboxOneWired[controller]->getButtonPress(b) >> 2); 
        }
        else
        {
            return (uint8_t)XboxOneWired[controller]->getButtonPress(b);
        }
    }
#endif

    return 0;
}

//Parse analog stick requests for each type of controller.
int16_t getAnalogHat(AnalogHatEnum a, uint8_t controller)
{
    if (Xbox360Wireless.Xbox360Connected[controller])
        return Xbox360Wireless.getAnalogHat(a, controller);

#ifdef SUPPORTWIREDXBOX360
    if (Xbox360Wired[controller]->Xbox360Connected)
    {
        int16_t val;
        val = Xbox360Wired[controller]->getAnalogHat(a);
        if (val == -32512) //8bitdo range fix
            val = -32768;
        return val;
    }

#endif

#ifdef SUPPORTWIREDXBOXONE
    if (XboxOneWired[controller]->XboxOneConnected)
        return XboxOneWired[controller]->getAnalogHat(a);
#endif

    return 0;
}

//Parse rumble activation requests for each type of controller.
void setRumbleOn(uint8_t lValue, uint8_t rValue, uint8_t controller)
{
    if (Xbox360Wireless.Xbox360Connected[controller])
        Xbox360Wireless.setRumbleOn(lValue, rValue, controller);

#ifdef SUPPORTWIREDXBOX360
    if (Xbox360Wired[controller]->Xbox360Connected)
    {
        Xbox360Wired[controller]->setRumbleOn(lValue, rValue); 
    }
#endif

#ifdef SUPPORTWIREDXBOXONE
    if (XboxOneWired[controller]->XboxOneConnected)
    {
        XboxOneWired[controller]->setRumbleOn(lValue / 8, rValue / 8, lValue / 2, rValue / 2);
    }
#endif
}

//Parse LED activation requests for each type of controller.
void setLedOn(LEDEnum led, uint8_t controller)
{
    if (Xbox360Wireless.Xbox360Connected[controller])
        Xbox360Wireless.setLedOn(led, controller);

#ifdef SUPPORTWIREDXBOX360
    if (Xbox360Wired[controller]->Xbox360Connected)
        Xbox360Wired[controller]->setLedOn(led);
#endif

#ifdef SUPPORTWIREDXBOXONE
    if (XboxOneWired[controller]->XboxOneConnected)
    {
        //no LEDs on Xbox One Controller. I think it is possible to adjust brightness but this is not implemented.
    }
#endif
}

bool controllerConnected(uint8_t controller)
{
    if (Xbox360Wireless.Xbox360Connected[controller])
        return 1;

#ifdef SUPPORTWIREDXBOX360
    if (Xbox360Wired[controller]->Xbox360Connected)
        return 1;
#endif

#ifdef SUPPORTWIREDXBOXONE
    if (XboxOneWired[controller]->XboxOneConnected)
        return 1;
#endif
    return 0;
}
#endif
