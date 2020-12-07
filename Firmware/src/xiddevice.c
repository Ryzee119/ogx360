/*
LUFA Library
Copyright (C) Dean Camera, 2013.

dean [at] fourwalledcubicle [dot] com
www.lufa-lib.org

Modified to XID device emulation by Ryzee119
*/

/*
Permission to use, copy, modify, distribute, and sell this
software and its documentation for any purpose is hereby granted
without fee, provided that the above copyright notice appear in
all copies and that both that the copyright notice and this
permission notice and warranty disclaimer appear in supporting
documentation, and that the name of the author not be used in
advertising or publicity pertaining to distribution of the
software without specific, written prior permission.

The author disclaims all warranties with regard to this
software, including all implied warranties of merchantability
and fitness.  In no event shall the author be liable for any
special, indirect or consequential damages or any damages
whatsoever resulting from loss of use, data or profits, whether
in an action of contract, negligence or other tortious action,
arising out of or in connection with the use or performance of
this software.
*/

#include "settings.h"
#include "xiddevice.h"
#include "dukecontroller.h"

#ifdef SUPPORTBATTALION
#include "steelbattalion.h"
#endif

extern bool enumerationComplete;
extern uint8_t ConnectedXID;

USB_XboxGamepad_Data_t PrevDukeHIDReportBuffer;

#ifdef SUPPORTBATTALION
USB_XboxSteelBattalion_Data_t PrevBattalionHIDReportBuffer;
#endif

/** LUFA HID Class driver interface configuration and state information. This structure is
passed to all HID Class driver functions, so that multiple instances of the same class
within a device can be differentiated from one another.
*/
USB_ClassInfo_HID_Device_t DukeController_HID_Interface = {
    .Config = {
        .InterfaceNumber = 0x00,
        .ReportINEndpoint = {
            .Address = 0x81,
            .Size = 20,
            .Banks = 1,
        },
        .PrevReportINBuffer = &PrevDukeHIDReportBuffer,
        .PrevReportINBufferSize = sizeof(PrevDukeHIDReportBuffer),
    },
};

#ifdef SUPPORTBATTALION
USB_ClassInfo_HID_Device_t SteelBattalion_HID_Interface = {
    .Config = {
        .InterfaceNumber = 0x00,
        .ReportINEndpoint = {
            .Address = 0x82,
            .Size = 26,
            .Banks = 1,
        },
        .PrevReportINBuffer = &PrevBattalionHIDReportBuffer,
        .PrevReportINBufferSize = sizeof(PrevBattalionHIDReportBuffer),
    },
};
#endif

/** Configures the board hardware and chip peripherals */
void SetupHardware(void)
{
    MCUSR &= ~(1 << WDRF);
    wdt_disable();
    clock_prescale_set(clock_div_1);
    USB_Init();
}

/** Event handler for the library USB Connection event. */
void EVENT_USB_Device_Connect(void)
{
}

/** Event handler for the library USB Disconnection event. */
void EVENT_USB_Device_Disconnect(void)
{
    enumerationComplete = false;
    digitalWrite(ARDUINO_LED_PIN, HIGH);
}

/** Event handler for the library USB Configuration Changed event. */
void EVENT_USB_Device_ConfigurationChanged(void)
{
    bool ConfigSuccess = true;
    switch (ConnectedXID)
    {
    case DUKE_CONTROLLER:
        ConfigSuccess &= HID_Device_ConfigureEndpoints(&DukeController_HID_Interface);
        ConfigSuccess &= Endpoint_ConfigureEndpoint(0x02, EP_TYPE_INTERRUPT, 6, 1); //Host Out endpoint opened manually for Duke.
        break;
#ifdef SUPPORTBATTALION
    case STEELBATTALION:
        ConfigSuccess &= HID_Device_ConfigureEndpoints(&SteelBattalion_HID_Interface);
        ConfigSuccess &= Endpoint_ConfigureEndpoint(0x01, EP_TYPE_INTERRUPT, 32, 1); //Host Out endpoint opened manually for SB.
        break;
#endif
    }
    USB_Device_EnableSOFEvents();
    enumerationComplete = ConfigSuccess;
}

/** Event handler for the library USB Control Request reception event. */
void EVENT_USB_Device_ControlRequest(void)
{
    //The Xbox Controller is a HID device, however it has some custom vendor requests
    //These are caught and processed here before going into the standard HID driver.
    //These are required for the controller to actually work on the console. Some games are more picky than others.
    //See http://xboxdevwiki.net/Xbox_Input_Devices under GET_DESCRIPTOR and GET_CAPABILITIES
    //The actual responses were obtained from a USB analyser when communicating with an OG Xbox console.

    if (USB_ControlRequest.bmRequestType == 0xC1)
    {
        if (USB_ControlRequest.bRequest == 0x06 && USB_ControlRequest.wValue == 0x4200)
        {
            Endpoint_ClearSETUP();
            switch (ConnectedXID)
            {
            case DUKE_CONTROLLER:
                Endpoint_Write_Control_Stream_LE(&DUKE_HID_DESCRIPTOR_XID, 16);
                break;
#ifdef SUPPORTBATTALION
            case STEELBATTALION:
                Endpoint_Write_Control_Stream_LE(&BATTALION_HID_DESCRIPTOR_XID, 16);
                break;
#endif
            }
            Endpoint_ClearOUT();
            return;
        }
        else if (USB_ControlRequest.bRequest == 0x01 && USB_ControlRequest.wValue == 0x0100)
        {
            Endpoint_ClearSETUP();
            switch (ConnectedXID)
            {
            case DUKE_CONTROLLER:
                Endpoint_Write_Control_Stream_LE(&DUKE_HID_CAPABILITIES_IN, 20);
                break;
#ifdef SUPPORTBATTALION
            case STEELBATTALION:
                Endpoint_Write_Control_Stream_LE(&BATTALION_HID_CAPABILITIES_IN, 21);
                break;
#endif
            }

            Endpoint_ClearOUT();
            return;
        }
        else if (USB_ControlRequest.bRequest == 0x01 && USB_ControlRequest.wValue == 0x0200)
        {
            Endpoint_ClearSETUP();
            switch (ConnectedXID)
            {
            case DUKE_CONTROLLER:
                Endpoint_Write_Control_Stream_LE(&DUKE_HID_CAPABILITIES_OUT, 6);
                break;
#ifdef SUPPORTBATTALION
            case STEELBATTALION:
                Endpoint_Write_Control_Stream_LE(&BATTALION_HID_CAPABILITIES_OUT, 22);
                break;
#endif
            }
            Endpoint_ClearOUT();
            return;
        }
    }

    //If the request is a standard HID control request, jump into the LUFA library to handle it for us.
    switch (ConnectedXID)
    {
    case DUKE_CONTROLLER:
        HID_Device_ProcessControlRequest(&DukeController_HID_Interface);
        break;
#ifdef SUPPORTBATTALION
    case STEELBATTALION:
        HID_Device_ProcessControlRequest(&SteelBattalion_HID_Interface);
        break;
#endif
    }
}

/** Event handler for the USB device Start Of Frame event. */
void EVENT_USB_Device_StartOfFrame(void)
{
    switch (ConnectedXID)
    {
    case DUKE_CONTROLLER:
        HID_Device_MillisecondElapsed(&DukeController_HID_Interface);
        break;
#ifdef SUPPORTBATTALION
    case STEELBATTALION:
        HID_Device_MillisecondElapsed(&SteelBattalion_HID_Interface);
        break;
#endif
    }
}

// HID class driver callback function for the creation of HID reports to the host.
bool CALLBACK_HID_Device_CreateHIDReport(USB_ClassInfo_HID_Device_t *const HIDInterfaceInfo,
                                         uint8_t *const ReportID, const uint8_t ReportType,
                                         void *ReportData, uint16_t *const ReportSize)
{

    USB_XboxGamepad_Data_t *DukeReport = (USB_XboxGamepad_Data_t *)ReportData;
#ifdef SUPPORTBATTALION
    USB_XboxSteelBattalion_Data_t *BattalionReport = (USB_XboxSteelBattalion_Data_t *)ReportData;
#endif

    switch (ConnectedXID)
    {
    case DUKE_CONTROLLER:
        DukeReport->startByte = 0x00;
        DukeReport->bLength = 20;
        DukeReport->dButtons = XboxOGDuke[0].dButtons;
        DukeReport->reserved = 0x00;
        DukeReport->A = XboxOGDuke[0].A;
        DukeReport->B = XboxOGDuke[0].B;
        DukeReport->X = XboxOGDuke[0].X;
        DukeReport->Y = XboxOGDuke[0].Y;
        DukeReport->BLACK = XboxOGDuke[0].BLACK;
        DukeReport->WHITE = XboxOGDuke[0].WHITE;
        DukeReport->L = XboxOGDuke[0].L;
        DukeReport->R = XboxOGDuke[0].R;
        DukeReport->leftStickX = XboxOGDuke[0].leftStickX;
        DukeReport->leftStickY = XboxOGDuke[0].leftStickY;
        DukeReport->rightStickX = XboxOGDuke[0].rightStickX;
        DukeReport->rightStickY = XboxOGDuke[0].rightStickY;
        *ReportSize = DukeReport->bLength;
        break;
#ifdef SUPPORTBATTALION
    case STEELBATTALION:
        BattalionReport->startByte = 0x00;
        BattalionReport->bLength = 26;
        BattalionReport->dButtons[0] = XboxOGSteelBattalion.dButtons[0];
        BattalionReport->dButtons[1] = XboxOGSteelBattalion.dButtons[1];
        BattalionReport->dButtons[2] = XboxOGSteelBattalion.dButtons[2];
        BattalionReport->aimingX = XboxOGSteelBattalion.aimingX;
        BattalionReport->aimingY = XboxOGSteelBattalion.aimingY;
        BattalionReport->rotationLever = XboxOGSteelBattalion.rotationLever;
        BattalionReport->sightChangeX = XboxOGSteelBattalion.sightChangeX;
        BattalionReport->sightChangeY = XboxOGSteelBattalion.sightChangeY;
        BattalionReport->leftPedal = XboxOGSteelBattalion.leftPedal;
        BattalionReport->middlePedal = XboxOGSteelBattalion.middlePedal;
        BattalionReport->rightPedal = XboxOGSteelBattalion.rightPedal;
        BattalionReport->tunerDial = XboxOGSteelBattalion.tunerDial;
        BattalionReport->gearLever = XboxOGSteelBattalion.gearLever;
        *ReportSize = BattalionReport->bLength;
        break;
#endif
    }

    return false;
}

/* HID class driver callback for the user processing of a received HID OUT report. This callback may fire in response to
*  either HID class control requests from the host, or by the normal HID endpoint polling procedure. Inside this callback
*  the user is responsible for the processing of the received HID output report from the host.*/
void CALLBACK_HID_Device_ProcessHIDReport(
    USB_ClassInfo_HID_Device_t *const HIDInterfaceInfo,
    const uint8_t ReportID, const uint8_t ReportType,
    const void *ReportData, const uint16_t ReportSize)
{
    //Only expect one HID report from the host and this is the actuator levels. The command is always 6 bytes long.
    //bit 3 is the left actuator value, bit 5 is the right actuator level.
    //See http://euc.jp/periphs/xbox-controller.en.html - Output Report
    if (ConnectedXID == DUKE_CONTROLLER && ReportSize == 0x06)
    {
        XboxOGDuke[0].left_actuator = ((uint8_t *)ReportData)[3];
        XboxOGDuke[0].right_actuator = ((uint8_t *)ReportData)[5];
        XboxOGDuke[0].rumbleUpdate = 1;
    }
}

//USB callback function for the processing of the device descriptors from the device.
//The standard HID descriptor requests are handled here.
uint16_t CALLBACK_USB_GetDescriptor(
    const uint16_t wValue, const uint16_t wIndex,
    const void **const DescriptorAddress)
{

    const uint8_t DescriptorType = (wValue >> 8);
    uint8_t nullString[1] = {0};
    const void *Address = NULL;
    uint16_t Size = NO_DESCRIPTOR;

    switch (DescriptorType)
    {
    case DTYPE_Device:
        switch (ConnectedXID)
        {
        case DUKE_CONTROLLER:
            Address = &DUKE_USB_DESCRIPTOR_DEVICE;
            Size = 18;
            break;
#ifdef SUPPORTBATTALION
        case STEELBATTALION:
            Address = &BATTALION_USB_DESCRIPTOR_DEVICE;
            Size = 18;
            break;
#endif
        }
        break;
    case DTYPE_Configuration:

        switch (ConnectedXID)
        {
        case DUKE_CONTROLLER:
            Address = &DUKE_USB_DESCRIPTOR_CONFIGURATION;
            Size = 32;
            break;
#ifdef SUPPORTBATTALION
        case STEELBATTALION:
            Address = &BATTALION_USB_DESCRIPTOR_CONFIGURATION;
            Size = 32;
            break;
#endif
        }
        break;
    case DTYPE_String:
        Address = &nullString; //OG Xbox controller doesn't use these.
        Size = NO_DESCRIPTOR;
        break;
    }
    *DescriptorAddress = Address;
    return Size;
}
