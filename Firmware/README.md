# ogx360 - Firmware

This folder contains the source code and precompiled firmware files. The source code is based on the [USB Host Shield Library](https://github.com/felis/USB_Host_Shield_2.0) for Arduino, and [LUFA USB AVR Framework](http://www.fourwalledcubicle.com/LUFA.php). The code was integrated and compiled using [Atmel Studio 7.0](https://www.microchip.com/mplab/avr-support/atmel-studio-7). It also relies on some Arduino libraries to allow the USB Host Shield Library to compile.
 
This software is released under GNU General Public License v3 (GPL-3). This following tables describes the different firmware options that are available (if unsure, just get the top two files)

| Firmware Files| Description |
|--|--|
| `ogx360_32u4_master.hex` | This is the default MASTER device firmware. This is the recommended firmware and contains support for 4 x Xbox 360 Wireless Controllers, Xbox 360 WiredControllers, Genuine Xbox One Wired Controllers, the [8bitdo wireless adaptor](http://www.8bitdo.com/wireless-usb-adapter/) and [ChronusMAX adaptor](https://cronusmax.com/). Refer to the respective websites for supported controllers. |
| `ogx360_32u4_slave.hex` | This is the default SLAVE device firmware. This must be programmed onto all the SLAVE modules (Player 2,3 and 4) if installed. |
| `ogx360_32u4_master_steelbattalion.hex` | Program this to the MASTER device only. This adds support for Steel Battalion Controller Emulation, but removes Wired Xbox One controller support. |
| `ogx360_debug.hex` | This is purely a test/debug firmware. Program this on the MASTER device to test your PCB if you have built one yourself. See instructions under **Testing** below. |


# Software Architecture

![Block Diagram](https://github.com/Ryzee119/ogx360/blob/master/Images/block_diagram.jpg?raw=true "ogx360-1")

# Programming
* Solder the Arduino Pro Micro Leonardo module into the ogx360 PCB slot you want to use if not already there.
* Download and install the [Arduino IDE]([https://www.arduino.cc/en/main/software](https://www.arduino.cc/en/main/software)). This wont be used, but comes with drivers that make the below process more reliable.
* Download and extract [avrdude](https://www.nongnu.org/avrdude/) to a working folder.
* Download [libusb0.dll](https://github.com/Ryzee119/ogx360/blob/master/Firmware/libusb0.dll) and place it in the avrdude working folder.
* Download the correct hex file from this repository or the one compiled yourself and place it in the avrdude folder. The folder should have an avrdude.conf, avrdude.exe, libusb0.dll and the hex files.
*Note that each module must be programmed individually, also the slave devices (player 2,3 and 4) must use the slave hex file. The master (player 1) must use the master hex file.*\
![avrdude folder](https://github.com/Ryzee119/ogx360/blob/master/Images/programming2.JPG?raw=true"ogx360-2")
* Connect the respective board you want to update into the USB port on the PC.
* Press the RESET button located on the ogx360 PCB. The programming bootloader should initialise and enumerate itself as a COM Port.
* Check which COM Port the device takes from your Device Manager. *Note: The bootloader is only active for a handful of seconds so if you're too slow just press RESET again. For example it should appear something like this:\
![COM Port](https://github.com/Ryzee119/ogx360/blob/master/Images/programming1.JPG?raw=true"ogx360-3")

* Once the COM Port number has been determined, edit COM17 and ogx360_32u4_master.hex in the below command to suit your setup. i.e. if you're programming a slave device `ogx360_32u4_master.hex` should be modified to `ogx360_32u4_slave.hex` If your COM port is COM5, edit COM17 to COM5.
* Press the RESET button again to restart to bootloader and give it a second to setup itself in windows, then promptly run the following command from the avrdude directory (Can be tricky timing):
`avrdude -C avrdude.conf -F -p atmega32u4 -c avr109 -b 57600 -P COM17 -Uflash:w:ogx360_32u4_master.hex:i`.\
A successful programming output should look something like this:\
![avrdude output](https://github.com/Ryzee119/ogx360/blob/master/Images/programming3.JPG?raw=true"ogx360-4")

* Repeat the process for any other modules you need to update.


# Compiling
* Download and install [Atmel Studio 7.0](https://www.microchip.com/mplab/avr-support/atmel-studio-7). Ensure you tick *'AVR 8-bit MCU'* support during the installation.
* Download the source code files from this repository and open `ogx360_32u4.atsln` with Atmel Studio 7.0.
* Check `settings.h` for compilation options, including master/slave, deadzone settings, additional controller support etc. *Note the programming bootloader takes up roughly 15% of the flash space. So if your compiled program exceeds approximately 85% of program memory usage it will fail programming.*
* Finally, to compile go to Build>Rebuild Solution. You should be presented with a Build succeeded message in the output console.
* On compilation the output .hex file is located in the project directory in /ogx360_32u4/Debug/ogx360_32u4.hex
* Program in accordance with the programming instructions above.

# Testing
If you have made the board yourself and want to check everything is healthy, I have added a test program `ogx360_debug.hex`. Program this to the master module as per the programming instructions under **Programming**.

The module will now appear as a keyboard and serial device in windows. Note the COM port number.

Open a serial monitoring program such as [Termite](https://www.compuphase.com/software_termite.htm), and set the following settings `Baud rate: 115200, 8 Data Bits, 1 Stop Bit, Parity None, Flowing Control RTS/CTS`. Once you connect to the COM Port, the self test should be performed. A healthy board with a Wireless Receiver connected should output the following:

![self test](https://github.com/Ryzee119/ogx360/blob/master/Images/programming5.JPG?raw=true"ogx360-5")

# References
The code comprises of the following libraries:
* LUFA USB Stack under the MIT license. http://www.fourwalledcubicle.com/files/LUFA/Doc/170418/html/_page__license_info.html
* USB Host Shield Library under the GNU General Public License. https://github.com/felis/USB_Host_Shield_2.0
* Arduino libraries have also been incorporates to allow compatability with the USB Host Shield library under the GNU Lesser General Public License. https://www.arduino.cc

By Ryzee119
