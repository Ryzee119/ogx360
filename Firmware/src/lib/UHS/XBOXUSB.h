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
 */

#ifndef _xboxusb_h_
#define _xboxusb_h_

#include "Usb.h"
#include "usbhid.h"
#include "xboxEnums.h"

/* Data Xbox 360 taken from descriptors */
#define EP_MAXPKTSIZE 32 // max size for data via USB

/* Names we give to the 3 Xbox360 pipes */
#define XBOX_CONTROL_PIPE 0
#define XBOX_INPUT_PIPE 1
#define XBOX_OUTPUT_PIPE 2

// PID and VID of the different devices
#define MICROSOFT_VID 0x045E    // Microsoft Corporation
#define HARMONIX_VID 0x1BAD     // Harmonix Music Systems, Inc.
#define JOYTECH_VID 0x162E      // Joytech Europe Ltd.
#define PDP_VID 0x0E6F          // Performance Designed Products (PDP), LLC
#define HONEYBEE_VID 0x12AB     // Honey Bee (Hong Kong) Limited
#define THRUSTMASTER_VID 0x24C6 // ThrustMaster, Inc.
#define MADCATZ_VID 0x0738      // Mad Catz Global Limited

#define XBOX_WIRED_PID 0x028E                         // Xbox 360 Wired controller
#define XBOX_WIRELESS_PID 0x028F                      // Xbox 360 Wireless Controller (Charging only)
#define XBOX_WIRELESS_RECEIVER_PID 0x0719             // Xbox 360 Wireless Gaming Receiver
#define XBOX_WIRELESS_RECEIVER_THIRD_PARTY_PID 0x0291 // Xbox 360 Wireless Gaming Receiver (Clone)
#define MADCATZ_WIRED_PID 0xF016                      // Mad Catz wired controller
#define MADCATZ_FIGHTSTICK_PID 0xF03A                 // MadCatz FightStick Neo
#define JOYTECH_WIRED_PID 0xBEEF                      // For Joytech wired controller
#define GAMESTOP_WIRED_PID1 0x0401                    // Gamestop wired controller
#define GAMESTOP_WIRED_PID2 0x0413                    // Gamestop wired controller
#define AFTERGLOW_WIRED_PID1 0x0213                   // Afterglow wired controller - it uses the same VID as a Gamestop controller
#define AFTERGLOW_WIRED_PID2 0x0302                   // Afterglow wired controller - it uses the same VID as a Gamestop controller
#define MADCATZ_PID1 0x4738                           // Street Fighter IV FightStick TE
#define MADCATZ_PID2 0xF018                           // Mad Catz Street Fighter IV SE
#define ROCKCANDY_WIRED_PID 0xFAFE                    // Rock candy wired controller
#define THRUSTMASTER_GPX_WIRED_PID 0x5B02             // Thrustmaster gpx controller
#define THRUSTMASTER_WHEEL_PID 0x5B00                 // Thrustmaster wheel controller
#define SF4_FIGHTPAD_PID 0x4728                       // Street fighter 4 fight pad stick controller
#define HORI_RAP_VXSA_PID 0xF502                      // Hori rap vx-sa stick controller
#define MADCATZ_BEAT_PAD 0x4740                       // MadCatz Beat Pad
#define KONAMI_DANCE_PAD 0x0004                       // Konami Dance Pad

#define XBOX_REPORT_BUFFER_SIZE 14 // Size of the input report buffer

//#define XBOX_MAX_ENDPOINTS   3

/** This class implements support for a Xbox wired controller via USB. */
class XBOXUSB : public USBDeviceConfig
{
public:
    /**
     * Constructor for the XBOXUSB class.
     * @param  pUsb   Pointer to USB class instance.
     */
    XBOXUSB(USB *pUsb);

    /** @name USBDeviceConfig implementation */
    /**
     * Initialize the Xbox Controller.
     * @param  parent   Hub number.
     * @param  port     Port number on the hub.
     * @param  lowspeed Speed of the device.
     * @return      0 on success.
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
        return ((vid == MICROSOFT_VID ||
                 vid == HARMONIX_VID ||
                 vid == JOYTECH_VID ||
                 vid == JOYTECH_VID ||
                 vid == PDP_VID ||
                 vid == HONEYBEE_VID ||
                 vid == THRUSTMASTER_VID ||
                 vid == MADCATZ_VID) &&

                (pid == XBOX_WIRED_PID ||
                 pid == MADCATZ_WIRED_PID ||
                 pid == MADCATZ_FIGHTSTICK_PID ||
                 pid == JOYTECH_WIRED_PID ||
                 pid == GAMESTOP_WIRED_PID1 ||
                 pid == GAMESTOP_WIRED_PID2 ||
                 pid == AFTERGLOW_WIRED_PID1 ||
                 pid == AFTERGLOW_WIRED_PID2 ||
                 pid == MADCATZ_PID1 ||
                 pid == MADCATZ_PID2 ||
                 pid == ROCKCANDY_WIRED_PID ||
                 pid == THRUSTMASTER_GPX_WIRED_PID ||
                 pid == THRUSTMASTER_WHEEL_PID ||
                 pid == SF4_FIGHTPAD_PID ||
                 pid == HORI_RAP_VXSA_PID ||
                 pid == MADCATZ_BEAT_PAD ||
                 pid == KONAMI_DANCE_PAD));
    };
    /**@}*/

    /** @name Xbox Controller functions */
    /**
     * getButtonPress(ButtonEnum b) will return true as long as the button is held down.
     *
     * While getButtonClick(ButtonEnum b) will only return it once.
     *
     * So you instance if you need to increase a variable once you would use getButtonClick(ButtonEnum b),
     * but if you need to drive a robot forward you would use getButtonPress(ButtonEnum b).
     * @param  b      ::ButtonEnum to read.
     * @return        getButtonClick(ButtonEnum b) will return a bool, while getButtonPress(ButtonEnum b) will return a byte if reading ::L2 or ::R2.
     */
    uint8_t getButtonPress(ButtonEnum b);
    bool getButtonClick(ButtonEnum b);
    /**@}*/

    /** @name Xbox Controller functions */
    /**
     * Return the analog value from the joysticks on the controller.
     * @param  a      Either ::LeftHatX, ::LeftHatY, ::RightHatX or ::RightHatY.
     * @return        Returns a signed 16-bit integer.
     */
    int16_t getAnalogHat(AnalogHatEnum a);

    /** Turn rumble off and all the LEDs on the controller. */
    void setAllOff()
    {
        setRumbleOn(0, 0);
        setLedRaw(0);
    };

    /** Turn rumble off the controller. */
    void setRumbleOff()
    {
        setRumbleOn(0, 0);
    };
    /**
     * Turn rumble on.
     * @param lValue     Left motor (big weight) inside the controller.
     * @param rValue     Right motor (small weight) inside the controller.
     */
    void setRumbleOn(uint8_t lValue, uint8_t rValue);
    /**
     * Set LED value. Without using the ::LEDEnum or ::LEDModeEnum.
     * @param value      See:
     * setLedOff(), setLedOn(LEDEnum l),
     * setLedBlink(LEDEnum l), and setLedMode(LEDModeEnum lm).
     */
    void setLedRaw(uint8_t value);

    /** Turn all LEDs off the controller. */
    void setLedOff()
    {
        setLedRaw(0);
    };
    /**
     * Turn on a LED by using ::LEDEnum.
     * @param l      ::OFF, ::LED1, ::LED2, ::LED3 and ::LED4 is supported by the Xbox controller.
     */
    void setLedOn(LEDEnum l);
    /**
     * Turn on a LED by using ::LEDEnum.
     * @param l      ::ALL, ::LED1, ::LED2, ::LED3 and ::LED4 is supported by the Xbox controller.
     */
    void setLedBlink(LEDEnum l);
    /**
     * Used to set special LED modes supported by the Xbox controller.
     * @param lm     See ::LEDModeEnum.
     */
    void setLedMode(LEDModeEnum lm);

    /**
     * Used to call your own function when the controller is successfully initialized.
     * @param funcOnInit Function to call.
     */
    void attachOnInit(void (*funcOnInit)(void))
    {
        pFuncOnInit = funcOnInit;
    };
    /**@}*/

    /** True if a Xbox 360 controller is connected. */
    bool Xbox360Connected;

    /** Used to limit the output transfer rate of the pipe**/
    uint32_t outPipeTimer = 0;

protected:
    /** Pointer to USB class instance. */
    USB *pUsb;
    /** Device address. */
    uint8_t bAddress;
    /** Endpoint info structure. */
    EpInfo epInfo[3];

private:
    /**
     * Called when the controller is successfully initialized.
     * Use attachOnInit(void (*funcOnInit)(void)) to call your own function.
     * This is useful for instance if you want to set the LEDs in a specific way.
     */
    void onInit();
    void (*pFuncOnInit)(void); // Pointer to function called in onInit()

    bool bPollEnable;

    /* Variables to store the buttons */
    uint32_t ButtonState;
    uint32_t OldButtonState;
    uint16_t ButtonClickState;
    int16_t hatValue[4];
    uint16_t controllerStatus;

    bool L2Clicked; // These buttons are analog, so we use we use these bools to check if they where clicked or not
    bool R2Clicked;

    uint8_t readBuf[EP_MAXPKTSIZE]; // General purpose buffer for input data
    uint8_t writeBuf[8];            // General purpose buffer for output data

    void readReport();  // read incoming data
    void printReport(); // print incoming date - Uncomment for debugging

    /* Private commands */
    void XboxCommand(uint8_t *data, uint16_t nbytes);
};
#endif
