# ogx360 - Firmware

The source code is uses [USB Host Shield Library](https://github.com/felis/USB_Host_Shield_2.0) for Arduino, and [LUFA USB AVR Framework](http://www.fourwalledcubicle.com/LUFA.php). It also relies on some Arduino libraries to allow the USB Host Shield Library to compile.

| Firmware Files| Description |
|--|--|
| `ogx360_32u4_master.hex` | This is the default MASTER device firmware. This is the recommended firmware and contains support for 4 x Xbox 360 Wireless Controllers, Xbox 360 WiredControllers, Genuine Xbox One Wired Controllers, the [8bitdo wireless adaptor](http://www.8bitdo.com/wireless-usb-adapter/) and [ChronusMAX adaptor](https://cronusmax.com/). Refer to the respective websites for supported controllers. |
| `ogx360_32u4_master_steelbattalion.hex` | Program this to the MASTER device only. This adds support for Steel Battalion Controller Emulation, but removes Wired Xbox One controller support. |
| `ogx360_32u4_slave.hex` | This is the default SLAVE device firmware. This must be programmed onto all the SLAVE modules (Player 2,3 and 4) if installed. |
| `ogx360_debug.hex` | This is purely a test/debug firmware. Program this on the MASTER device to test your PCB if you have built one yourself. See instructions under **Testing** below. |

# Programming (Platform IO IDE)
* Download and install [Arduino IDE](https://www.arduino.cc/en/software) for the required drivers.
* Setup Visual Studio Code as per the Compiling instructions.
* Select the project environment from the Platform IO toolbar. SLAVE/MASTER etc.
* Prepare to press the RESET button on the ogx360 PCB.
* Hit the program button on the Platform IO toolbar (`→`).
* When the says `Waiting for the new upload port...` bit the RESET button.
* It can help to hold the RESET button while compiling then release when it is waiting for the upload port.
* Repeat the process for any other modules you need to update.

# Programming (from HEX file)
* Download and install [Arduino IDE](https://www.arduino.cc/en/software) for the required drivers.
* Download [avrdude](http://download.savannah.gnu.org/releases/avrdude/). Windows [download](http://download.savannah.gnu.org/releases/avrdude/avrdude-6.3-mingw32.zip).
* Create a file called `program.bat` in the same folder as `avrdude.exe` and copy this command into it. Note press RESET on the ogx360 and determine what COM number it appears in the device manager and alter the COM17 below as required.
* `avrdude -C avrdude.conf -F -p atmega32u4 -c avr109 -b 57600 -P COM17 -Uflash:w:ogx360_32u4_master.hex:i`
* Press RESET on the ogx360, wait a second then run the bat file.

# Compiling
* Download and install [Arduino IDE](https://www.arduino.cc/en/software) for the required drivers.
* Download and install [Visual Studio Code](https://code.visualstudio.com/).
* Install the [PlatformIO IDE](https://platformio.org/platformio-ide) plugin.
* Download/clone this repo.
* In Visual Studio Code `File > Open Folder... > ogx360/Firmware`
* Hit build on the Platform IO toolbar (`✓`).
* This will build slave, master, and master with steelbattalion. See `.pio/build/` folder for the different hex files.

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
