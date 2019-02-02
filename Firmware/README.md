# ogx360 - Firmware

This folder contains the source code. The source code is based on the [USB Host Shield Library](https://github.com/felis/USB_Host_Shield_2.0) for Arduino, and [LUFA USB AVR Framework](http://www.fourwalledcubicle.com/LUFA.php). The code was integrated and compiled using [Atmel Studio 7.0](https://www.microchip.com/mplab/avr-support/atmel-studio-7). It relys on some Arduino libraries to allow the USB Host Shield Library to compile.
 
This software is released under GNU General Public License v3 (GPL-3).

# Software Overview
The software is written in C++ with links to C libraries. It is compiled in Atmel Studio 7. The microcontrollers are Atmega32U4's which have a built in USB 2.0 device Controller. The Arduino Pro Micro Leonardo boards are used as modules in this design to easily incorporate this functionality. One module is required for each Player that you wish to support. i.e 4 modules are required for 4 player support. The USB Host Controller is a MAX3421 which enumerates and communicates with the Xbox 360 wireless receiver.

Player 1 module is the 'master'. This communicates with the USB Host Controller and gathers data from the Xbox 360 wireless receiver for all four wireless controllers if they are synced. This data is then collated and sent to Player 2,3 and 4 slave devices, where they are converted to HID game controller reports that are compatible with the Original Xbox. The 'master' module will generate its own HID report for Player 1.
This main data bus is bi-directional and facilitates button states, axis states and controller rumble values for all four controllers.

This may sound slow but all this happens every 4 milliseconds. The same rate that a genuine Xbox controller is polled from the console so there should be no perceivable difference in input lag from a genuine controller.

![Block Diagram](https://github.com/Ryzee119/ogx360/blob/master/Images/block_diagram.jpg?raw=true "ogx360-1")

# Testing
If you have made the board yourself and want to check everything is healthy, I have added a test program `ogx360_debug.hex`. Program this to the master module as per the programming instructions under **Programming**.

The module will now appear as a keyboard/serial composite device in Windows. Note the COM port number. See below:

![comm port](https://github.com/Ryzee119/ogx360/blob/master/Images/programming4.png?raw=true"ogx360-5")

Open a serial monitoring program such as [Termite](https://www.compuphase.com/software_termite.htm), and set the following settings `Baud rate: 115200, 8 Data Bits, 1 Stop Bit, Parity None, Flowing Control RTS/CTS`. Once you connect to the commport, the self test should be performed. A healthy board with a Wireless Receiver connected should output the following:

![self test](https://github.com/Ryzee119/ogx360/blob/master/Images/programming5.JPG?raw=true"ogx360-6")

# Compiling
* Download and install [Atmel Studio 7.0](https://www.microchip.com/mplab/avr-support/atmel-studio-7). Ensure you tick *'AVR 8-bit MCU'* support during the installation.
* Download the source code files from this repository and open `ogx360_32u4.atsln` with Atmel Studio 7.0.
* To compile for the master device, uncomment `#define HOST` from main.cpp line 23.
* To compile for the slave devices, comment out `#define HOST` from main.cpp line 23. i.e. `//#define HOST`.
* Finally, to compile go to Build>Build Solution. You should be presented with a Build succeeded message in the output console.
* On compilation the output .hex file is located in the project directory in /ogx360_32u4/Debug/ogx360_32u4.hex

# Programming
* Place the Arduino Leonardo Pro Micro Leonardo board into the ogx360 PCB slot you want to use.
* Download and extract [avrdude](https://www.nongnu.org/avrdude/) to a working folder.
* Download [libusb0.dll](https://github.com/Ryzee119/ogx360/blob/master/Firmware/libusb0.dll) and place it in the avrdude working folder.
* Download the correct hex file from this repository or the one compiled yourself and place it in the avrdude folder. The folder should have an avrdude.conf, avrdude.exe, libusb0.dll and the hex files.
*Note that each module must be programmed individually, also the slave devices (player 2,3 and 4) must use the slave hex file. The master (player 1) must use the master hex file.*

![avrdude folder](https://github.com/Ryzee119/ogx360/blob/master/Images/programming2.JPG?raw=true"ogx360-2")

* Connect the respective board you want to update into the USB port on the PC.
* Press the RESET button located on the ogx360 PCB. The programming bootloader should initialise and enumerate itself as a COM Port.
* Check which COM Port the device takes from your Device Manager. *Note: The bootloader is only active for a handful of seconds so if you're too slow just press RESET again.*

![COM Port](https://github.com/Ryzee119/ogx360/blob/master/Images/programming1.JPG?raw=true"ogx360-3")

* Once the COM Port number has been determined, edit COM17 and ogx360_32u4_master.hex in the below command to suit your requirements. i.e. if you're programming a slave device `ogx360_32u4_master.hex` should be modified to `ogx360_32u4_slave.hex`
* Press the RESET button again to restart to bootloader and run the following command from the avrdude directory:
`avrdude -C avrdude.conf -F -p atmega32u4 -c avr109 -b 57600 -P COM17 -Uflash:w:ogx360_32u4_master.hex:i`.

![avrdude output](https://github.com/Ryzee119/ogx360/blob/master/Images/programming3.JPG?raw=true"ogx360-4")

* Repeat the process for any other modules you need to update.
* If you're having issues with avrdude, you can try installing the latest version of the Arduino IDE which contains all the relevant drivers and try running avrdude again after installation.
* 

# Original Xbox Controller USB Protocol
The Original Xbox Controllers actually have a inbuilt 3 port USB hub. One channel is connected to the controller and the other two connect to the expansion ports on top of the controller.

The controller basically is a standard 'Human Input Device Class' as defined by the [USB Definition](https://www.usb.org/hid), however implement some custom vendor requests and are required for the controller to be detected correctly on an Original Xbox Console. The Original Xbox controller also does not have a HID Report Descriptor, consequently will not work in a Windows machine via a USB adaptor without custom drivers.
My code does not emulate the USB Hub and enumerates as the controller directly. The Xbox console doesn't seem to care.

A device descriptor dump from my Original Controller can be seen [here](https://github.com/Ryzee119/ogx360/tree/master/Firmware/DescriptorDump_VID045E_PID0289.md). There's a few variations around the world it seems, infact my own two controllers had slightly different descriptors but I guess all would work the same.
The device descriptor and the below responses are coded in `xboxcontroller.c`.

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
| 02 | 0x00 - bcdXid, Always 0x00 |
| 03 | 0x01 - bType, 1=Xbox Gamecontroller? |
| 04 | 0x01 - bSubType, 0x01 = Gamepad? |
| 05 | 0x02 - bMaxInputReportSize? is what I got from my logic analyser |
| 06 | 0x14 - bMaxOutputReportSize? is what I got from my logic analyser|
| 07 | 0x06 - wAlternateProductIds? is what I got from my logic analyser|
| 08 to 15 | 8 x 0xFF - Controller pads the remaining with 0xFF's for some reason. Just what I saw on my USB Analyser |

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

# References
The code comprises of the following libraries:
LUFA USB Stack under the MIT license. http://www.fourwalledcubicle.com/files/LUFA/Doc/170418/html/_page__license_info.html

USB Host Shield Library under the GNU General Public License. https://github.com/felis/USB_Host_Shield_2.0

Arduino libraries have also been incorporates to allow compatability with the USB Host Shield library under the GNU Lesser General Public License. https://www.arduino.cc


The following library modifications have been made: (These are usually comments with 'Ryzee' in the respective files)
The following modifications to the Arduino libraries have been made:

Wire.cpp, line 272,273 and 274. Added code to the TwiWire::flush function.

twi.c, line 62-84. twi_timeout function added, line 95, 184, 186, 225, 227, 267, 269, 311, 313, 411, 413


The following modifications to the USB Host Shield Library have been made:

XBOXRECV.cpp, Poll() function, removed the checkStatus() call. This is done in main.cpp now.

XBOXRECV.cpp, checkStatus() added an input uint8_t status to toggle between battery and controller status.

XBOXRECV.h, checkStatus() function made a public function (see the .h file).





By Ryzee119
