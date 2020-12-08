/* Copyright (C) 2012 Kristian Lauszus, TKJ Electronics. All rights reserved.

 This software may be distributed and modified under the terms of the GNU
 General Public License version 2 (GPL2) as published by the Free Software
 Foundation and appearing in the file GPL2.TXT included in the packaging of
 this file. Please note that GPL2 Section 2[b] requires that all works based
 on this software must also be made publicly available under the terms of
 the GPL2 ("Copyleft").

 Contact information
 -------------------

 Kristian Lauszus, TKJ Electronics
 Web      :  http://www.tkjelectronics.com
 e-mail   :  kristianl@tkjelectronics.com

 getBatteryLevel and checkStatus functions made by timstamp.co.uk found using BusHound from Perisoft.net
 */

#ifndef _xboxrecv_h_
#define _xboxrecv_h_

#include "Usb.h"
#include "xboxEnums.h"

/* Data Xbox 360 taken from descriptors */
#define EP_MAXPKTSIZE 32 // max size for data via USB

/* Names we give to the 9 Xbox360 pipes */
#define XBOX_CONTROL_PIPE 0
#define XBOX_INPUT_PIPE_1 1
#define XBOX_OUTPUT_PIPE_1 2
#define XBOX_INPUT_PIPE_2 3
#define XBOX_OUTPUT_PIPE_2 4
#define XBOX_INPUT_PIPE_3 5
#define XBOX_OUTPUT_PIPE_3 6
#define XBOX_INPUT_PIPE_4 7
#define XBOX_OUTPUT_PIPE_4 8

#define XBOX_INPUT_PIPE_1_CHATPAD 9

#define XBOX_MAX_ENDPOINTS 17

enum ChatPadButton
{
        //Offset byte 26 or 27. You can get 2 buttons are once on the chatpad,
        CHATPAD_1 = 23,
        CHATPAD_2 = 22,
        CHATPAD_3 = 21,
        CHATPAD_4 = 20,
        CHATPAD_5 = 19,
        CHATPAD_6 = 18,
        CHATPAD_7 = 17,
        CHATPAD_8 = 103,
        CHATPAD_9 = 102,
        CHATPAD_0 = 101,

        CHATPAD_Q = 39,
        CHATPAD_W = 38,
        CHATPAD_E = 37,
        CHATPAD_R = 36,
        CHATPAD_T = 35,
        CHATPAD_Y = 34,
        CHATPAD_U = 33,
        CHATPAD_I = 118,
        CHATPAD_O = 117,
        CHATPAD_P = 100,

        CHATPAD_A = 55,
        CHATPAD_S = 54,
        CHATPAD_D = 53,
        CHATPAD_F = 52,
        CHATPAD_G = 51,
        CHATPAD_H = 50,
        CHATPAD_J = 49,
        CHATPAD_K = 119,
        CHATPAD_L = 114,
        CHATPAD_COMMA = 98,

        CHATPAD_Z = 70,
        CHATPAD_X = 69,
        CHATPAD_C = 68,
        CHATPAD_V = 67,
        CHATPAD_B = 66,
        CHATPAD_N = 65,
        CHATPAD_M = 82,
        CHATPAD_PERIOD = 83,
        CHATPAD_ENTER = 99,

        CHATPAD_LEFT = 85,
        CHATPAD_SPACE = 84,
        CHATPAD_RIGHT = 81,
        CHATPAD_BACK = 113,

        //Offset byte 25,
        CHATPAD_SHIFT = 1,
        CHATPAD_GREEN = 2,
        CHATPAD_ORANGE = 4,
        CHATPAD_MESSENGER = 8,
};

#define CHATPAD_LED_CAPSLOCK_OFF 0x00
#define CHATPAD_LED_GREEN_OFF 0x01
#define CHATPAD_LED_ORANGE_OFF 0x02
#define CHATPAD_LED_MESSENGER_OFF 0x03
#define CHATPAD_LED_CAPSLOCK_ON 0x08
#define CHATPAD_LED_GREEN_ON 0x09
#define CHATPAD_LED_ORANGE_ON 0x0A
#define CHATPAD_LED_MESSENGER_ON 0x0B

/**
 * This class implements support for a Xbox Wireless receiver.
 *
 * Up to four controllers can connect to one receiver, if more is needed one can use a second receiver via the USBHub class.
 */
class XBOXRECV : public USBDeviceConfig
{
public:
        /**
         * Constructor for the XBOXRECV class.
         * @param  pUsb   Pointer to USB class instance.
         */
        XBOXRECV(USB *pUsb);

        /** @name USBDeviceConfig implementation */
        /**
         * Address assignment and basic initilization is done here.
         * @param  parent   Hub number.
         * @param  port     Port number on the hub.
         * @param  lowspeed Speed of the device.
         * @return          0 on success.
         */
        uint8_t ConfigureDevice(uint8_t parent, uint8_t port, bool lowspeed);
        /**
         * Initialize the Xbox wireless receiver.
         * @param  parent   Hub number.
         * @param  port     Port number on the hub.
         * @param  lowspeed Speed of the device.
         * @return          0 on success.
         */
        uint8_t Init(uint8_t parent, uint8_t port, bool lowspeed);
        /**
         * Release the USB device.
         * @return 0 on success.
         */
        uint8_t Release();
        /**
         * Poll the USB Input endpoins and run the state machines.
         * @return 0 on success.
         */
        uint8_t Poll();

        /**
         * Get the device address.
         * @return The device address.
         */
        virtual uint8_t GetAddress()
        {
                return bAddress;
        };

        /**
         * Used to check if the controller has been initialized.
         * @return True if it's ready.
         */
        virtual bool isReady()
        {
                return bPollEnable;
        };

        /**
         * Used by the USB core to check what this driver support.
         * @param  vid The device's VID.
         * @param  pid The device's PID.
         * @return     Returns true if the device's VID and PID matches this driver.
         */
        virtual bool VIDPIDOK(uint16_t vid, uint16_t pid)
        {
                return 0;
        };
        /**@}*/

        /** @name Xbox Controller functions */
        /**
         * getButtonPress(uint8_t controller, ButtonEnum b) will return true as long as the button is held down.
         *
         * While getButtonClick(uint8_t controller, ButtonEnum b) will only return it once.
         *
         * So you instance if you need to increase a variable once you would use getButtonClick(uint8_t controller, ButtonEnum b),
         * but if you need to drive a robot forward you would use getButtonPress(uint8_t controller, ButtonEnum b).
         * @param  b          ::ButtonEnum to read.
         * @param  controller The controller to read from. Default to 0.
         * @return            getButtonClick(uint8_t controller, ButtonEnum b) will return a bool, while getButtonPress(uint8_t controller, ButtonEnum b) will return a byte if reading ::L2 or ::R2.
         */
        uint8_t getButtonPress(ButtonEnum b, uint8_t controller = 0);
        bool getButtonClick(ButtonEnum b, uint8_t controller = 0);
        /**@}*/

        /** @name Xbox Controller functions */
        /**
         * Return the analog value from the joysticks on the controller.
         * @param  a          Either ::LeftHatX, ::LeftHatY, ::RightHatX or ::RightHatY.
         * @param  controller The controller to read from. Default to 0.
         * @return            Returns a signed 16-bit integer.
         */
        int16_t getAnalogHat(AnalogHatEnum a, uint8_t controller = 0);

        /**
         * Used to disconnect any of the controllers.
         * @param controller The controller to disconnect. Default to 0.
         */
        void disconnect(uint8_t controller = 0);

        /**
         * Turn rumble off and all the LEDs on the specific controller.
         * @param  controller The controller to write to. Default to 0.
         */
        void setAllOff(uint8_t controller = 0)
        {
                setRumbleOn(0, 0, controller);
                setLedOff(controller);
        };

        /**
         * Turn rumble off the specific controller.
         * @param  controller The controller to write to. Default to 0.
         */
        void setRumbleOff(uint8_t controller = 0)
        {
                setRumbleOn(0, 0, controller);
        };
        /**
         * Turn rumble on.
         * @param lValue     Left motor (big weight) inside the controller.
         * @param rValue     Right motor (small weight) inside the controller.
         * @param controller The controller to write to. Default to 0.
         */
        void setRumbleOn(uint8_t lValue, uint8_t rValue, uint8_t controller = 0);
        /**
         * Set LED value. Without using the ::LEDEnum or ::LEDModeEnum.
         * @param value      See:
         * setLedOff(uint8_t controller), setLedOn(uint8_t controller, LED l),
         * setLedBlink(uint8_t controller, LED l), and setLedMode(uint8_t controller, LEDMode lm).
         * @param controller The controller to write to. Default to 0.
         */
        void setLedRaw(uint8_t value, uint8_t controller = 0);

        /**
         * Turn all LEDs off the specific controller.
         * @param controller The controller to write to. Default to 0.
         */
        void setLedOff(uint8_t controller = 0)
        {
                setLedRaw(0, controller);
        };
        /**
         * Turn on a LED by using ::LEDEnum.
         * @param l          ::OFF, ::LED1, ::LED2, ::LED3 and ::LED4 is supported by the Xbox controller.
         * @param controller The controller to write to. Default to 0.
         */
        void setLedOn(LEDEnum l, uint8_t controller = 0);
        /**
         * Turn on a LED by using ::LEDEnum.
         * @param l          ::ALL, ::LED1, ::LED2, ::LED3 and ::LED4 is supported by the Xbox controller.
         * @param controller The controller to write to. Default to 0.
         */
        void setLedBlink(LEDEnum l, uint8_t controller = 0);
        /**
         * Used to set special LED modes supported by the Xbox controller.
         * @param lm         See ::LEDModeEnum.
         * @param controller The controller to write to. Default to 0.
         */
        void setLedMode(LEDModeEnum lm, uint8_t controller = 0);
        /**
         * Used to get the battery level from the controller.
         * @param  controller The controller to read from. Default to 0.
         * @return            Returns the battery level as an integer in the range of 0-3.
         */
        uint8_t getBatteryLevel(uint8_t controller = 0);
        /**
         * Used to check if a button has changed.
         * @param  controller The controller to read from. Default to 0.
         * @return            True if a button has changed.
         */
        bool buttonChanged(uint8_t controller = 0);

        /**
         * Used to call your own function when the controller is successfully initialized.
         * @param funcOnInit Function to call.
         */
        void attachOnInit(void (*funcOnInit)(void))
        {
                pFuncOnInit = funcOnInit;
        };

        void checkControllerPresence(uint8_t controller = 0); //Ryzee - moved function to public and split into two functions.
        void checkControllerBattery(uint8_t controller = 0);  //Ryzee - moved function to public and split into two functions.

        void enableChatPad(uint8_t controller);                       //Ryzee
        void chatPadKeepAlive1(uint8_t controller);                   //Ryzee
        void chatPadKeepAlive2(uint8_t controller);                   //Ryzee
        uint8_t getChatPadPress(ChatPadButton b, uint8_t controller); //Ryzee
        uint8_t getChatPadClick(ChatPadButton b, uint8_t controller); //Ryzee
        void chatPadQueueLed(uint8_t led, uint8_t controller);        //Ryzee
        uint8_t chatPadLedQueue[4][4];                                //You can queue up 4 LED commands
        uint8_t chatPadInitNeeded[4];

        /**@}*/

        /** True if a wireless receiver is connected. */
        bool XboxReceiverConnected;
        /** Variable used to indicate if the XBOX 360 controller is successfully connected. */
        uint8_t Xbox360Connected[4];

protected:
        /** Pointer to USB class instance. */
        USB *pUsb;
        /** Device address. */
        uint8_t bAddress;
        /** Endpoint info structure. */
        EpInfo epInfo[XBOX_MAX_ENDPOINTS];
        uint8_t chatpadEnabled;

private:
        /**
         * Called when the controller is successfully initialized.
         * Use attachOnInit(void (*funcOnInit)(void)) to call your own function.
         * This is useful for instance if you want to set the LEDs in a specific way.
         * @param controller The initialized controller.
         */
        void onInit(uint8_t controller);
        void (*pFuncOnInit)(void); // Pointer to function called in onInit()

        bool bPollEnable;

        /* Variables to store the buttons */
        uint32_t ButtonState[4];
        uint32_t OldButtonState[4];
        uint16_t ButtonClickState[4];
        bool buttonStateChanged[4]; // True if a button has changed

        /* Variables to store the chatpad buttons */
        uint32_t ChatPadState[4];
        uint32_t OldChatPadState[4];
        uint32_t ChatPadClickState[4];
        bool ChatPadStateChanged[4]; // True if a chatpad button has changed

        int16_t hatValue[4][4];
        uint16_t controllerStatus[4];

        bool L2Clicked[4]; // These buttons are analog, so we use we use these bools to check if they where clicked or not
        bool R2Clicked[4];

        uint32_t checkStatusTimer; //Timing for checkStatus() signals
        uint32_t chatPadLedTimer;  //Timing for chat pad led updates

        uint8_t readBuf[EP_MAXPKTSIZE]; // General purpose buffer for input data
        uint8_t writeBuf[12];           // General purpose buffer for output data

        void readReport(uint8_t controller);                  // read incoming data
        void printReport(uint8_t controller, uint8_t nBytes); // print incoming date - Uncomment for debugging

        /* Private commands */
        void XboxCommand(uint8_t controller, uint8_t *data, uint16_t nbytes);
        void chatPadProcessLed(uint8_t controller);
        //void checkStatus(); moved to public function - Ryzee
};
#endif
