/*
LUFA Library
Copyright (C) Dean Camera, 2013.

dean [at] fourwalledcubicle [dot] com
www.lufa-lib.org
*/

/*
Copyright 2013  Dean Camera (dean [at] fourwalledcubicle [dot] com)

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

#include "xboxcontroller.h"

USB_XboxGamepad_Data_t XboxOG[4]; //Xbox gamepad data structure to store all button and actuator states for all four controllers.
uint8_t PrevXboxHIDReportBuffer[sizeof(USB_XboxGamepad_Data_t)]; //Need to store the previous controller state aswell.
bool enumerationComplete=false; //Just a fla which is set when enumeration has been completed by the host.

/** LUFA HID Class driver interface configuration and state information. This structure is
passed to all HID Class driver functions, so that multiple instances of the same class
within a device can be differentiated from one another.
*/
USB_ClassInfo_HID_Device_t Xbox_HID_Interface = {
	.Config = {
		.InterfaceNumber          = 0x00,
		.ReportINEndpoint         =	{
			.Address              = 0x81,
			.Size                 = 0x14,
			.Banks                = 1,
		},
		.PrevReportINBuffer           = PrevXboxHIDReportBuffer,
		.PrevReportINBufferSize       = sizeof(PrevXboxHIDReportBuffer)-15, //Last 15 bytes aren't part of the HID report. I use them for other things
	},
};


//Obtained from USB dump of original controller
//USB Device descriptor
const uint8_t PROGMEM USB_DESCRIPTOR_DEVICE[] = {
	0x12,   //bLength - length of packet in bytes
	0x01,   //bDescriptorType - 0x01 = Device Descriptor
	0x10, 0x01, //bcdUSB - 2 bytes. Sets USB Spec 1.1 (0110)
	0x00,   //bDeviceClass
	0x00,   //bDeviceSubClass
	0x00,   //dDeviceProtocol
	0x20,   //bMaxPacketSize
	0x5E, 0x04, //Vendor ID (LSB First) = 0x045E
	0x89, 0x02, //Product ID (LSB First) = 0x0289
	0x21, 0x01, //bcdDevice - 1.21
	0x00,   //iManufacturer = none
	0x00,   //iProduct = none
	0x00,   //iSerialNumber = none
	0x01    //bNumConfigurations = 1
};

//Obtained from USB dump of original controller
//Usb Configuration Descriptor
const uint8_t PROGMEM USB_DESCRIPTOR_CONFIGURATION[] = {
	//Configuration Descriptor//
	0x09,			//bLength of config descriptor
	0x02,			//bDescriptorType, 2=Configuration Descriptor
	0x20, 0x00,		//wTotalLength 2-bytes, total length (including interface and endpoint descriptors)
	0x01,			//bNumInterfaces, just 1
	0x01,			//bConfigurationValue
	0x00,			//iConfiguration - index to string descriptors. we dont use them
	0x80,			//bmAttributes - 0x80 = USB Bus Powered
	0xFA,			//bMaxPower - maximum power in 2mA units. 0xFA=500mA. Genuine OG controller is normally 100mA (0x32)
	
	//Interface Descriptor//
	0x09, //bLength of interface descriptor
	0x04, //bDescriptorType, 4=Interface  Descriptor
	0x00, //bInterfaceNumber
	0x00, //bAlternateSetting
	0x02, //bNumEndpoints - we have two endpoints (IN for button presses, and OUT for rumble values)
	0x58, //bInterfaceClass - From OG Xbox controller
	0x42, //bInterfaceSubClass - From OG Xbox controller
	0x00, //bInterfaceProtocol
	0x00, //iInterface - index to string descriptors. we dont use them
	
	//Endpoint Descriptor (IN)//
	0x07, //bLength of endpoint descriptor
	0x05, //bDescriptorType, 5=Endpoint Descriptor
	0x81, //bEndpointAddress, Address=1, Direction IN
	0x03, //bmAttributes, 3=Interrupt Endpoint
	0x20, 0x00, //wMaxPacketSize
	0x04, //bInterval, Interval for polling the interrupt endpoint. 4ms
	
	//Endpoint Descriptor (OUT)//
	0x07, //bLength of endpoint descriptor
	0x05, //bDescriptorType, 5=Endpoint Descriptor
	0x02, //bEndpointAddress, Address=2, Direction OUT
	0x03, //bmAttributes, 3=Interrupt Endpoint
	0x20, 0x00, //wMaxPacketSize
	0x04 //bInterval, Interval for polling the interrupt endpoint. 4ms
};

//Obtained from USB analyser dump of original controller when talking to console
//This is a custom vendor request specific to the xbox controller and an OG xbox.
const uint8_t HID_DESCRIPTOR_XID[] = {
	0x10,   //bLength - Length of report. 16 bytes
	0x42,   //bDescriptorType - always 0x42
	0x00,   //bcdXid
	0x01,   //bType - 1=Xbox Gamecontroller?
	0x01,   //bSubType, 0x01 = Gamepad? This is what I got from my logic analyser
	0x02,   //bMaxInputReportSize //0x02 from logic analyser
	0x14,   //bMaxOutputReportSize - Controller button report - 20 bytes
	0x06,   //wAlternateProductIds - 0x06 from logic analyser
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF //The OG controller has this padding for some reason
}; 


//It will have bits set (1) where the bit is valid in the controller button report.
//If the bit is auto-generated, it will be cleared (0). Refer http://xboxdevwiki.net/Xbox_Input_Devices
//Obtained from a USB analyser dump when talking with console.
const uint8_t HID_CAPABILITIES_IN[]  = {
	0x00,   //Always 0x00
	0x14,   //bLength - length of packet in bytes
	0xFF,
	0x00,   //This byte is 0x00 because this particular byte is not used in the button report.
	0xFF,
	0xFF, 0xFF, 0xFF,
	0xFF, 0xFF, 0xFF,
	0xFF, 0xFF, 0xFF,
	0xFF, 0xFF, 0xFF,
	0xFF, 0xFF, 0xFF
};


//It will have bits set (1) where the bit is valid in the controller rumnble report.
//Obtained from a USB analyser dump when talking with console.
const uint8_t HID_CAPABILITIES_OUT[] = {
	0x00, //Always 0x00
	0x06, //bLength - length of packet in bytes
	0xFF, 0xFF,0xFF, 0xFF //bits corresponding to the rumble bits. all 0xFF as they are used.
};





/** Configures the board hardware and chip peripherals for the demo's functionality. */
void SetupHardware(void)
{
	/* Disable watchdog if enabled by bootloader/fuses */
	MCUSR &= ~(1 << WDRF);
	wdt_disable();

	/* Disable clock division */
	clock_prescale_set(clock_div_1);

	/* Hardware Initialization */
	USB_Init();

}

/** Event handler for the library USB Connection event. */
void EVENT_USB_Device_Connect(void){
	
}

/** Event handler for the library USB Disconnection event. */
void EVENT_USB_Device_Disconnect(void){
	enumerationComplete=false;
	digitalWrite(PLAYER_LED_PIN, LOW);
	digitalWrite(ARDUINO_LED_PIN, HIGH);
}

/** Event handler for the library USB Configuration Changed event. */
void EVENT_USB_Device_ConfigurationChanged(void)
{
	bool ConfigSuccess = true;
	ConfigSuccess &= HID_Device_ConfigureEndpoints(&Xbox_HID_Interface);
	USB_Device_EnableSOFEvents();
	enumerationComplete=ConfigSuccess;
}


/** Event handler for the library USB Control Request reception event. */
void EVENT_USB_Device_ControlRequest(void)
{

	//The Xbox Controller is a HID device, however it has some custom vendor requests
	//These are caught and processed here before going into the standard HID driver.
	//These are required for the controller to actually work on the console. Some games are more picky than others.
	//See http://xboxdevwiki.net/Xbox_Input_Device under GET_DESCRIPTOR and GET_CAPABILITIES
	//The actual responses were obtained from a USB analyser when communicating with an Xbox console.
	if (USB_ControlRequest.bmRequestType == 0xC1 && USB_ControlRequest.bRequest == 0x06 && USB_ControlRequest.wValue == 0x4200) {
		Endpoint_ClearSETUP();
		Endpoint_Write_Control_Stream_LE(&HID_DESCRIPTOR_XID, 0x10);
		Endpoint_ClearOUT();
		return;
	}
	else if (USB_ControlRequest.bmRequestType == 0xC1 && USB_ControlRequest.bRequest == 0x01 && USB_ControlRequest.wValue == 0x0100) {
		Endpoint_ClearSETUP();
		Endpoint_Write_Control_Stream_LE(&HID_CAPABILITIES_IN, 0x14);
		Endpoint_ClearOUT();
		return;
	}
	else if (USB_ControlRequest.bmRequestType == 0xC1 && USB_ControlRequest.bRequest == 0x01 && USB_ControlRequest.wValue == 0x0200) {
		Endpoint_ClearSETUP();
		Endpoint_Write_Control_Stream_LE(&HID_CAPABILITIES_OUT, 0x06);
		Endpoint_ClearOUT();
		return;
	}
	//If the request is a standard HID control request, jump into the LUFA library to handle it for us.
	HID_Device_ProcessControlRequest(&Xbox_HID_Interface);
}


/** Event handler for the USB device Start Of Frame event. */
void EVENT_USB_Device_StartOfFrame(void)
{
	HID_Device_MillisecondElapsed(&Xbox_HID_Interface);
}


// HID class driver callback function for the creation of HID reports to the host.
bool CALLBACK_HID_Device_CreateHIDReport(USB_ClassInfo_HID_Device_t* const HIDInterfaceInfo,
uint8_t* const ReportID,
const uint8_t ReportType,
void* ReportData,
uint16_t* const ReportSize)
{
	USB_XboxGamepad_Data_t* XboxReport = (USB_XboxGamepad_Data_t*)ReportData;
	XboxReport->startByte = 0x00;
	XboxReport->bLength = 0x14;
	XboxReport->digButtons = XboxOG[0].digButtons;
	XboxReport->reserved = 0x00;
	XboxReport->A = XboxOG[0].A;
	XboxReport->B = XboxOG[0].B;
	XboxReport->X = XboxOG[0].X;
	XboxReport->Y = XboxOG[0].Y;
	XboxReport->BLACK = XboxOG[0].BLACK;
	XboxReport->WHITE = XboxOG[0].WHITE;
	XboxReport->L = XboxOG[0].L;
	XboxReport->R = XboxOG[0].R;
	XboxReport->leftStickX = XboxOG[0].leftStickX;
	XboxReport->leftStickY = XboxOG[0].leftStickY;
	XboxReport->rightStickX = XboxOG[0].rightStickX;
	XboxReport->rightStickY = XboxOG[0].rightStickY;
	*ReportSize = XboxReport->bLength;
	return false;
}


//HID class driver callback function for the processing of HID reports from the device.
void CALLBACK_HID_Device_ProcessHIDReport(USB_ClassInfo_HID_Device_t* const HIDInterfaceInfo,
const uint8_t ReportID,
const uint8_t ReportType,
const void* ReportData,
const uint16_t ReportSize)
{
	//Only expect one HID report from the host and this is the actuator levels. The command is always 6 bytes long.
	//bit 3 is the left actuator value, bit 5 is the right actuator level.
	//See http://euc.jp/periphs/xbox-controller.en.html - Output Report
	if (ReportSize == 0x06) {
		XboxOG[0].left_actuator = ((uint8_t *)ReportData)[3];
		XboxOG[0].right_actuator = ((uint8_t *)ReportData)[5];
		XboxOG[0].rumbleUpdate = 1;
	}
	//I made 0xFF an error value because I found if there was issue with comms I would often get 0xFF back.
	//So now I can distinguish between a real 0xFF value and a 0xFF value received in error.
	if(XboxOG[0].left_actuator==0xFF){
		XboxOG[0].left_actuator-=1; 
	}
	if(XboxOG[0].right_actuator==0xFF){
		XboxOG[0].right_actuator-=1;
	}
}

//USB callback function for the processing of the device descriptors from the device.
//The standard HID descriptor requests are handled here.
uint16_t CALLBACK_USB_GetDescriptor(const uint16_t wValue, const uint16_t wIndex, const void** const DescriptorAddress) {
	const uint8_t  DescriptorType   = (wValue >> 8);
	uint8_t nullString[1] = {0};
	const void* Address = NULL;
	uint16_t    Size    = NO_DESCRIPTOR;

	switch (DescriptorType)
	{
		case DTYPE_Device:
		Address = &USB_DESCRIPTOR_DEVICE;
		Size    = 18;
		break;
		case DTYPE_Configuration:
		Address = &USB_DESCRIPTOR_CONFIGURATION;
		Size    = 32;
		break;
		case DTYPE_String:
		Address = &nullString; //OG Xbox controller doesn't use these.
		Size    = NO_DESCRIPTOR;
		break;
	}
	*DescriptorAddress = Address;
	return Size;
}
