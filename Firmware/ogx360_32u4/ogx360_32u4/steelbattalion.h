/*
* steelbattalion.h
*
* Created: 9/02/2019 4:18:14 PM
* Author: Ryan
*/


#ifndef STEELBATTALION_H_
#define STEELBATTALION_H_

#ifdef SUPPORTBATTALION


/* Digital Button Masks  thanks to Cxbx reloaded for saving me typing it all out myself.
https://github.com/Cxbx-Reloaded/Cxbx-Reloaded/blob/develop/src/core/hle/XAPI/Xapi.h */
#define SBC_GAMEPAD_W0_RIGHTJOYMAINWEAPON 0x0001
#define SBC_GAMEPAD_W0_RIGHTJOYFIRE 0x0002
#define SBC_GAMEPAD_W0_RIGHTJOYLOCKON 0x0004
#define SBC_GAMEPAD_W0_EJECT 0x0008
#define SBC_GAMEPAD_W0_COCKPITHATCH 0x0010
#define SBC_GAMEPAD_W0_IGNITION 0x0020
#define SBC_GAMEPAD_W0_START 0x0040
#define SBC_GAMEPAD_W0_MULTIMONOPENCLOSE 0x0080
#define SBC_GAMEPAD_W0_MULTIMONMAPZOOMINOUT 0x0100
#define SBC_GAMEPAD_W0_MULTIMONMODESELECT 0x0200
#define SBC_GAMEPAD_W0_MULTIMONSUBMONITOR 0x0400
#define SBC_GAMEPAD_W0_MAINMONZOOMIN 0x0800
#define SBC_GAMEPAD_W0_MAINMONZOOMOUT 0x1000
#define SBC_GAMEPAD_W0_FUNCTIONFSS 0x2000
#define SBC_GAMEPAD_W0_FUNCTIONMANIPULATOR 0x4000
#define SBC_GAMEPAD_W0_FUNCTIONLINECOLORCHANGE 0x8000

#define SBC_GAMEPAD_W1_WASHING 0x0001
#define SBC_GAMEPAD_W1_EXTINGUISHER 0x0002
#define SBC_GAMEPAD_W1_CHAFF 0x0004
#define SBC_GAMEPAD_W1_FUNCTIONTANKDETACH 0x0008
#define SBC_GAMEPAD_W1_FUNCTIONOVERRIDE 0x0010
#define SBC_GAMEPAD_W1_FUNCTIONNIGHTSCOPE 0x0020
#define SBC_GAMEPAD_W1_FUNCTIONF1 0x0040
#define SBC_GAMEPAD_W1_FUNCTIONF2 0x0080
#define SBC_GAMEPAD_W1_FUNCTIONF3 0x0100
#define SBC_GAMEPAD_W1_WEAPONCONMAIN 0x0200
#define SBC_GAMEPAD_W1_WEAPONCONSUB 0x0400
#define SBC_GAMEPAD_W1_WEAPONCONMAGAZINE 0x0800
#define SBC_GAMEPAD_W1_COMM1 0x1000
#define SBC_GAMEPAD_W1_COMM2 0x2000
#define SBC_GAMEPAD_W1_COMM3 0x4000
#define SBC_GAMEPAD_W1_COMM4 0x8000

#define SBC_GAMEPAD_W2_COMM5 0x0001
#define SBC_GAMEPAD_W2_LEFTJOYSIGHTCHANGE 0x0002
#define SBC_GAMEPAD_W2_TOGGLEFILTERCONTROL 0x0004
#define SBC_GAMEPAD_W2_TOGGLEOXYGENSUPPLY 0x0008
#define SBC_GAMEPAD_W2_TOGGLEFUELFLOWRATE 0x0010
#define SBC_GAMEPAD_W2_TOGGLEBUFFREMATERIAL 0x0020
#define SBC_GAMEPAD_W2_TOGGLEVTLOCATION 0x0040

typedef struct
{
	uint8_t startByte;
	uint8_t bLength;
	uint16_t dButtons[3];
	int16_t aimingX;
	int16_t aimingY;
	int16_t rotationLever;
	int16_t sightChangeX;
	int16_t sightChangeY;
	uint16_t leftPedal;
	uint16_t middlePedal;
	uint16_t rightPedal;
	int8_t tunerDial;
	int8_t gearLever;

} USB_XboxSteelBattalion_Data_t;


typedef struct
{
	uint8_t startByte;
	uint8_t bLength;
	uint8_t CockpitHatch_EmergencyEject;
	uint8_t Start_Ignition;
	uint8_t MapZoomInOut_OpenClose;
	uint8_t SubMonitorModeSelect_ModeSelect;
	uint8_t MainMonitorZoomOut_MainMonitorZoomIn;
	uint8_t Manipulator_ForecastShootingSystem;
	uint8_t Washing_LineColorChange;
	uint8_t Chaff_Extinguisher;
	uint8_t Override_TankDetach;
	uint8_t F1_NightScope;
	uint8_t F3_F2;
	uint8_t SubWeaponControl_MainWeaponControl;
	uint8_t Comm1_MagazineChange;
	uint8_t Comm3_Comm2;
	uint8_t Comm5_Comm4;
	uint8_t GearR_;
	uint8_t Gear1_GearN;
	uint8_t Gear3_Gear2;
	uint8_t Gear5_Gear4;
	uint8_t dummy;
} USB_XboxSteelBattalion_Feedback_t;


//Obtained from USB dump of original controller
//USB Device descriptor
const uint8_t PROGMEM BATTALION_USB_DESCRIPTOR_DEVICE[] = {
	0x12, //bLength - length of packet in bytes
	0x01, //bDescriptorType - 0x01 = Device Descriptor
	0x10, 0x01, //bcdUSB - 2 bytes. Sets USB Spec 1.1 (0110)
	0x00, //bDeviceClass
	0x00, //bDeviceSubClass
	0x00, //dDeviceProtocol
	0x20, //bMaxPacketSize
	0x7B, 0x0A, //Vendor ID (LSB First) = 0x0A7B
	0x00, 0xD0, //Product ID (LSB First) = 0xD000
	0x00, 0x01, //bcdDevice - 0100
	0x00, //iManufacturer = none
	0x00, //iProduct = none
	0x00, //iSerialNumber = none
	0x01 //bNumConfigurations = 1
};

const uint8_t PROGMEM BATTALION_USB_DESCRIPTOR_CONFIGURATION[] = {
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
	0x04, //bDescriptorType, 4=Interface Descriptor
	0x00, //bInterfaceNumber
	0x00, //bAlternateSetting
	0x02, //bNumEndpoints - we have two endpoints (IN for button presses, and OUT for leds and feedback)
	0x58, //bInterfaceClass
	0x42, //bInterfaceSubClass
	0x00, //bInterfaceProtocol
	0x00, //iInterface - index to string descriptors. we dont use them
	
	//Endpoint Descriptor (IN)//
	0x07, //bLength of endpoint descriptor
	0x05, //bDescriptorType, 5=Endpoint Descriptor
	0x82, //bEndpointAddress, Address=2, Direction IN
	0x03, //bmAttributes, 3=Interrupt Endpoint
	0x20, 0x00, //wMaxPacketSize 32 bytes
	0x04, //bInterval, Interval for polling the interrupt endpoint. 4ms
	
	//Endpoint Descriptor (OUT)//
	0x07, //bLength of endpoint descriptor
	0x05, //bDescriptorType, 5=Endpoint Descriptor
	0x01, //bEndpointAddress, Address=1, Direction OUT
	0x03, //bmAttributes, 3=Interrupt Endpoint
	0x20, 0x00, //wMaxPacketSize 32 bytes
	0x04 //bInterval, Interval for polling the interrupt endpoint. 4ms
};

//Obtained from USB analyser dump of original controller when talking to console
//This is a custom vendor request specific to the xbox controller and an OG xbox.
const uint8_t BATTALION_HID_DESCRIPTOR_XID[] = {
	0x10, //bLength - Length of report. 16 bytes
	0x42, //bDescriptorType - always 0x42
	0x00, 0x01, //bcdXid
	0x80, //bType - 1=Xbox Gamecontroller, 0x80 = STEELBATTALION
	0x01, //bSubType, 0x01 = Gamepad (Duke), 0x02 = Gamepad S/Steel Battalion, 0x10, Wheel, 0x20, Arcade Stick etc.
	0x1A, //bMaxInputReportSize //HID Report from steel battalion controller - 26 bytes
	0x16, //bMaxOutputReportSize - feedback commands report from host - 22bytes
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF //wAlternateProductIds
};


//It will have bits set (1) where the bit is valid in the controller button report.
//If the bit is auto-generated, it will be cleared (0). Refer http://xboxdevwiki.net/Xbox_Input_Devices
//Not sure if this is required but added it just in case.
//Ive just guessed what it should be.
const uint8_t BATTALION_HID_CAPABILITIES_IN[] = {
	0x00, //Always 0x00
	26, //bLength - length of packet in bytes
	0xFF,
	0xFF,
	0xFF,
	0xFF, 0xFF, 0xFF,
	0xFF, 0xFF, 0xFF,
	0xFF, 0xFF, 0xFF,
	0xFF, 0xFF, 0xFF,
	0xFF, 0xFF, 0xFF,
	0xFF, 0xFF, 0xFF,
	0xFF, 0xFF, 0xFF
};


//It will have bits set (1) where the bit is valid in the controller rumnble report.
//Not sure if this is required but added it just in case.
//Ive just guessed what it should be.
const uint8_t BATTALION_HID_CAPABILITIES_OUT[] = {
	0x00, //Always 0x00
	22, //bLength - length of packet in bytes
	0xFF, 0xFF,0xFF, 0xFF,
	0xFF, 0xFF,0xFF, 0xFF,
	0xFF, 0xFF,0xFF, 0xFF,
	0xFF, 0xFF,0xFF, 0xFF,
	0xFF, 0xFF,0xFF, 0xFF,
	0xFF, 0xFF,0xFF, 0xFF,
	0xFF, 0xFF,0xFF, 0xFF,
	0xFF, 0xFF,0xFF, 0xFF,
	0xFF, 0xFF,0xFF, 0xFF,
	0xFF, 0xFF,0xFF, 0xFF,
	0xFF
};

#endif

#endif /* STEELBATTALION_H_ */