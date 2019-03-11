
# Steel Battalion USB Protocol Notes
The device descriptor and the below responses are coded in `steelbattalion.h`.\
Huge shoutout to @Kekule over on the ogxbox.com forums who provided the descriptor dump and logic analyser dump from his Steel Battalion controller!

### USB Controller Descriptor
Information for device USB\Vendor_0A7B_Product_D000:


Connection Information:\
------------------------------\
Device current bus speed: FullSpeed\
Device supports USB 1.1 specification\
Device supports USB 2.0 specification\
Device address: 0x0010\
Current configuration value: 0x00\
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
0x0A7B	idVendor\
0xD000	idProduct\
0x0100	bcdDevice\
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
0xFA	bMaxPower      (500 mA)

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
0x82	bEndpointAddress  (IN endpoint 2)\
0x03	bmAttributes      (Transfer: Interrupt / Synch: None / Usage: Data)\
0x0020	wMaxPacketSize    (1 x 32 bytes)\
0x04	bInterval         (4 frames)

Endpoint Descriptor:\
------------------------------\
0x07	bLength\
0x05	bDescriptorType\
0x01	bEndpointAddress  (OUT endpoint 1)\
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
| 01 | 0x1A - Size of the report. 0x1A = 26 bytes long |
| 02/03 | Each bit in the byte represents the Controller digital buttons.*|
| 04/05 | Each bit in the byte represents the Controller digital buttons.*|
| 06/07 | Each bit in the byte represents the Controller digital buttons.*|
| 08/09 | Aiming Stick X-Axis (Analog, signed short 16-bit) |
| 10/11 | Aiming Stick Y-Axis (Analog, signed short 16-bit) |
| 12/13 | Rotation Lever (Analog, signed short 16-bit) |
| 14/15 | Sight Change Stick X-Axis (Analog, signed short 16-bit) |
| 16/17 | Sight Change Stick Y-Axis (Analog, signed short 16-bit) |
| 18/19 | Left Pedal 0x0000 to 0xFF00  |
| 20/21 | Middle Pedal 0x0000 to 0xFF00 |
| 22/23 | Right Pedal 0x0000 to 0xFF00 |
| 24 | Tuner Dial Position (0-15 starting at the 9 o'clock position going clockwise|
| 25 | Gear Lever Position (7-13 R,N,1,2,3,4,5)|

\* Refer to http://xboxdevwiki.net/Xbox_Input_Devices#Steel_Battalion_Controller

## Feedback Data (The data that is sent from the host to the Steel Battalion Controller. These are used to light up the LEDs)
Feedback data is 0-F to which controls the brightness of the LED. Each byte feedbacks two variables. The upper and lower nibble are used.\
This is send to the OUT endpoint. Normally endpoint address 0x01.

| byte | Data |
| --- | --- |
| 00 | 0x00 - Always 0x00 |
| 01 | 0x16 - Size of the report. 22 bytes long |
| 02 | CockpitHatch / EmergencyEject |
| 03 | Start / Ignition |
| 04 | MapZoomInOut / OpenClose |
| 05 | SubMonitorModeSelect / ModeSelect |
| 06 | MainMonitorZoomOut / MainMonitorZoomIn |
| 07 | Manipulator / ForecastShootingSystem |
| 08 | Washing / LineColorChange |
| 09 | Chaff / Extinguisher |
| 10 | Override / TankDetach |
| 11 | F1 / NightScope |
| 12 | F3 / F2 |
| 13 | SubWeaponControl / MainWeaponControl |
| 14 | Comm1 / MagazineChange |
| 15 | Comm3 / Comm2 |
| 16 | Comm5 / Comm4 |
| 17 | GearR / 00 |
| 18 | Gear1 / GearN |
| 19 | Gear3 / Gear2 |
| 20 | Gear5 / Gear4 |
| 21 | Not Used |


## XID Descriptor - Custom Vendor Request
This is what was obtained when monitoring the USB bus between the controller and the console.
The controller responds with this when `bmRequestType = 0xC1, bRequest = 0x06 and wValue = 0x4200` from the host.

| byte | Data |
| --- | --- |
| 00 | 0x10 - bLength, Length of report. 16 bytes |
| 01 | 0x42 - bDescriptorType, always 0x42 |
| 02/03 | 0x00, 0x01 - bcdXid  |
| 04 | 0x80 - bType - 0x80 = Steel Battalion |
| 05 | 0x01 - bSubType, 0x01 = Gamepad |
| 06 | 0x1A - bMaxInputReportSize |
| 07 | 0x16 - bMaxOutputReportSize |
| 08 to 15 | 8 x 0xFF - wAlternateProductIds |