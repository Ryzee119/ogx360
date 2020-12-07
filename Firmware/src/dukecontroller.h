
/*
 * dukecontroller.h
 *
 * Created: 9/02/2019 4:02:59 PM
 *  Author: Ryan
 */

#ifndef DUKECONTROLLER_H_
#define DUKECONTROLLER_H_

/* Digital Button Masks */
#define DUP (1 << 0)
#define DDOWN (1 << 1)
#define DLEFT (1 << 2)
#define DRIGHT (1 << 3)
#define START_BTN (1 << 4)
#define BACK_BTN (1 << 5)
#define LS_BTN (1 << 6)
#define RS_BTN (1 << 7)

typedef struct
{
    uint8_t startByte; //Always 0x00
    uint8_t bLength;
    uint8_t dButtons;
    uint8_t reserved;
    uint8_t A;
    uint8_t B;
    uint8_t X;
    uint8_t Y;
    uint8_t BLACK;
    uint8_t WHITE;
    uint8_t L;
    uint8_t R;
    int16_t leftStickX;
    int16_t leftStickY;
    int16_t rightStickX;
    int16_t rightStickY;
    //These last few values aren't part of the xbox controller HID report, but are added here by me to store extra stuff.
    uint8_t left_actuator;
    uint8_t right_actuator;
    uint8_t rumbleUpdate;
} USB_XboxGamepad_Data_t;

//Obtained from USB dump of original controller
//USB Device descriptor
const uint8_t PROGMEM DUKE_USB_DESCRIPTOR_DEVICE[] = {
    0x12,       //bLength - length of packet in bytes
    0x01,       //bDescriptorType - 0x01 = Device Descriptor
    0x10, 0x01, //bcdUSB - 2 bytes. Sets USB Spec 1.1 (0110)
    0x00,       //bDeviceClass
    0x00,       //bDeviceSubClass
    0x00,       //dDeviceProtocol
    0x20,       //bMaxPacketSize
    0x5E, 0x04, //Vendor ID (LSB First) = 0x045E
    0x89, 0x02, //Product ID (LSB First) = 0x0289
    0x21, 0x01, //bcdDevice - 1.21
    0x00,       //iManufacturer = none
    0x00,       //iProduct = none
    0x00,       //iSerialNumber = none
    0x01        //bNumConfigurations = 1
};

//Obtained from USB dump of original controller
//Usb Configuration Descriptor
const uint8_t PROGMEM DUKE_USB_DESCRIPTOR_CONFIGURATION[] = {
    //Configuration Descriptor//
    0x09,       //bLength of config descriptor
    0x02,       //bDescriptorType, 2=Configuration Descriptor
    0x20, 0x00, //wTotalLength 2-bytes, total length (including interface and endpoint descriptors)
    0x01,       //bNumInterfaces, just 1
    0x01,       //bConfigurationValue
    0x00,       //iConfiguration - index to string descriptors. we dont use them
    0x80,       //bmAttributes - 0x80 = USB Bus Powered
    0xFA,       //bMaxPower - maximum power in 2mA units. 0xFA=500mA. Genuine OG controller is normally 100mA (0x32)

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
    0x07,       //bLength of endpoint descriptor
    0x05,       //bDescriptorType, 5=Endpoint Descriptor
    0x81,       //bEndpointAddress, Address=1, Direction IN
    0x03,       //bmAttributes, 3=Interrupt Endpoint
    0x20, 0x00, //wMaxPacketSize
    0x04,       //bInterval, Interval for polling the interrupt endpoint. 4ms

    //Endpoint Descriptor (OUT)//
    0x07,       //bLength of endpoint descriptor
    0x05,       //bDescriptorType, 5=Endpoint Descriptor
    0x02,       //bEndpointAddress, Address=2, Direction OUT
    0x03,       //bmAttributes, 3=Interrupt Endpoint
    0x20, 0x00, //wMaxPacketSize
    0x04        //bInterval, Interval for polling the interrupt endpoint. 4ms
};

//Obtained from USB analyser dump of original controller when talking to console
//This is a custom vendor request specific to the xbox controller and an OG xbox.
const uint8_t DUKE_HID_DESCRIPTOR_XID[] = {
    0x10,                                          //bLength - Length of report. 16 bytes
    0x42,                                          //bDescriptorType - always 0x42
    0x00, 0x01,                                    //bcdXid
    0x01,                                          //bType - 1=Xbox Gamecontroller
    0x02,                                          //bSubType, 0x02 = Gamepad S, 0x01 = Gamepad (Duke)
    0x14,                                          //bMaxInputReportSize //HID Report from controller - 20 bytes
    0x06,                                          //bMaxOutputReportSize - Rumble report from host - 6 bytes
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF //wAlternateProductIds
};

//It will have bits set (1) where the bit is valid in the controller button report.
//If the bit is auto-generated, it will be cleared (0). Refer http://xboxdevwiki.net/Xbox_Input_Devices
//Obtained from a USB analyser dump when talking with console.
const uint8_t DUKE_HID_CAPABILITIES_IN[] = {
    0x00, //Always 0x00
    0x14, //bLength - length of packet in bytes
    0xFF,
    0x00, //This byte is 0x00 because this particular byte is not used in the button report.
    0xFF,
    0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF};

//It will have bits set (1) where the bit is valid in the controller rumnble report.
//Obtained from a USB analyser dump when talking with console.
const uint8_t DUKE_HID_CAPABILITIES_OUT[] = {
    0x00,                  //Always 0x00
    0x06,                  //bLength - length of packet in bytes
    0xFF, 0xFF, 0xFF, 0xFF //bits corresponding to the rumble bits. all 0xFF as they are used.
};

#endif