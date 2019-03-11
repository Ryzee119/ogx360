
# Original Xbox Controller USB Protocol Notes
The Original Xbox Controllers actually have a inbuilt 3 port USB hub. One channel is connected to the controller and the other two connect to the expansion ports on top of the controller.

The controller basically is a standard 'Human Input Device Class' as defined by the [USB Definition](https://www.usb.org/hid), however implement some custom vendor requests and are required for the controller to be detected correctly on an Original Xbox Console. The Original Xbox controller also does not have a HID Report Descriptor, consequently will not work in a Windows machine via a USB adaptor without custom drivers.
My code does not emulate the USB Hub and enumerates as the controller directly. The Xbox console doesn't seem to care.

The device descriptor and the below responses are coded in `dukecontroller.h`.

### USB Controller Descriptor
Information for device USB\Vendor_045E_Product_0289:


Connection Information:\
------------------------------\
Device current bus speed: FullSpeed\
Device supports USB 1.1 specification\
Device address: 0x000B\
Current configuration value: 0x01\
Number of open pipes: 2


Device Descriptor:\
------------------------------\
0x12	bLength\
0x01	bDescriptorType\
0x0110	bcdUSB\
0x00	bDeviceClass\
0x00	bDeviceSubClass\
0x00	bDeviceProtocol\
0x08	bMaxPacketSize0   (8 bytes)\
0x045E	idVendor\
0x0289	idProduct\
0x0121	bcdDevice\
0x00	iManufacturer\
0x00	iProduct\
0x00	iSerialNumber\
0x01	bNumConfigurations


Configuration Descriptor:\
------------------------------\
0x09	bLength\
0x02	bDescriptorType\
0x0020	wTotalLength   (32 bytes)\
0x01	bNumInterfaces\
0x01	bConfigurationValue\
0x00	iConfiguration\
0x80	bmAttributes   (Bus-powered Device)\
0x32	bMaxPower      (100 mA)


Interface Descriptor:\
------------------------------\
0x09	bLength\
0x04	bDescriptorType\
0x00	bInterfaceNumber\
0x00	bAlternateSetting\
0x02	bNumEndPoints\
0x58	bInterfaceClass\
0x42	bInterfaceSubClass\
0x00	bInterfaceProtocol\
0x00	iInterface


Endpoint Descriptor:\
------------------------------\
0x07	bLength\
0x05	bDescriptorType\
0x81	bEndpointAddress  (IN endpoint 1)\
0x03	bmAttributes      (Transfer: Interrupt / Synch: None / Usage: Data)\
0x0020	wMaxPacketSize    (1 x 32 bytes)\
0x04	bInterval         (4 frames)


Endpoint Descriptor:\
------------------------------\
0x07	bLength\
0x05	bDescriptorType\
0x02	bEndpointAddress  (OUT endpoint 2)\
0x03	bmAttributes      (Transfer: Interrupt / Synch: None / Usage: Data)\
0x0020	wMaxPacketSize    (1 x 32 bytes)\
0x04	bInterval         (4 frames)

Microsoft OS Descriptor is not available. Error code: 0x0000001F

String Descriptor Table\
--------------------------------\
Index  LANGID  String\
0x00   0x0000




### HID Input Report (The data that is sent to the host when the host requests the HID Report)
| byte | Data |
| --- | --- |
| 00 | 0x00 - Always 0x00 |
| 01 | 0x14 - Size of the report. 0x14 = 20 bytes long |
| 02 | Each bit in the byte represents the Controller digital buttons.*|
| 03 | 0x00 - Always 0x00 |
| 04 | A Button (Analog button, unsigned char) 0x00 to 0xFF |
| 05 | B Button (Analog button, unsigned char) 0x00 to 0xFF |
| 06 | X Button (Analog button, unsigned char) 0x00 to 0xFF |
| 07 | Y Button (Analog button, unsigned char) 0x00 to 0xFF |
| 08 | BLACK Button (Analog button, unsigned char) 0x00 to 0xFF |
| 09 | WHITE Button (Analog button, unsigned char) 0x00 to 0xFF |
| 10 | L Trigger (Analog button, unsigned char) 0x00 to 0xFF |
| 11 | R Trigger (Analog button, unsigned char) 0x00 to 0xFF |
| 12/13 | Left Stick X-Axis (Analog, signed short 16-bit) |
| 14/15 | Left Stick Y-Axis (Analog, signed short 16-bit) |
| 16/17 | Right Stick X-Axis (Analog, signed short 16-bit) |
| 18/19 | Right Stick Y-Axis (Analog, signed short 16-bit) |

\* bit 0 to bit 7 = D-Pad Up, D-Pad Down, D-Pad Left, D-Pad Right, Start, Back, Left Stick Button, Right Stick Button

## HID Output Report (The data that is sent from the host. i.e rumble updates)
| byte | Data |
| --- | --- |
| 00 | 0x00 - Always 0x00 |
| 01 | 0x06 - Size of the report. 6 bytes long |
| 02 | 0x00 - Always 0x00 |
| 03 | Left Rumble Value (Analog, unsigned char) 0x00 to 0xFF |
| 04 | 0x00 - Always 0x00 |
| 05 | Right Rumble Value (Analog, unsigned char) 0x00 to 0xFF |

## XID Descriptor - Custom Vendor Request
This is what I obtained when monitoring the USB bus between the controller and the console.
The controller responds with this when `bmRequestType = 0xC1, bRequest = 0x06 and wValue = 0x4200` from the host.

| byte | Data |
| --- | --- |
| 00 | 0x10 - bLength, Length of report. 16 bytes |
| 01 | 0x42 - bDescriptorType, always 0x42 |
| 02 & 03 | 0x00, 0x01 - bcdXid  |
| 04 | 0x01 - bType - 1=Xbox Gamecontroller, 0x80 = Steel Battalion |
| 05 | 0x02 - bSubType, 0x01 = Gamepad (Duke), 0x02 = Gamepad S/Steel Battalion, 0x10 = Wheel.. |
| 06 | 0x14 - bMaxInputReportSize |
| 07 | 0x06 - bMaxOutputReportSize |
| 08 to 15 | 8 x 0xFF - wAlternateProductIds |

## HID Capabilities IN - Custom Vendor Request
This is what I obtained when monitoring the USB bus between the controller and the console.
The controller responds with this when `bmRequestType = 0xC1, bRequest = 0x01 and wValue = 0x0100` from the host.
It will have bits set (1) where the bit is valid in the HID Input Report.
If the bit is auto-generated, it will be cleared (0). Refer http://xboxdevwiki.net/Xbox_Input_Devices

| byte | Data |
| --- | --- |
| 00 | 0x00 - Always 0x00 |
| 01 | 0x14 - Size of the report. 0x14 = 20 bytes long |
| 02 | 0xFF |
| 03 | 0x00 |
| 04 to 19 | 16 x 0xFF |

## HID Capabilities OUT - Custom Vendor Request
This is what I obtained when monitoring the USB bus between the controller and the console.
The controller responds with this when `bmRequestType = 0xC1, bRequest = 0x01 and wValue = 0x0200` from the host.
It will have bits set (1) where the bit is valid in the HID Output Report.
If the bit is auto-generated, it will be cleared (0). Refer http://xboxdevwiki.net/Xbox_Input_Devices

| byte | Data |
| --- | --- |
| 00 | 0x00 - Always 0x00 |
| 01 | 0x06 - Size of the report. 6 bytes long |
| 02 to 05 | 4 x 0xFF |