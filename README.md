


# ogx360 - Overview

The ogx360 is a small circuit board which allows you to use Wireless Xbox 360 Controllers via an Xbox 360 Wireless Receiver on the Original Xbox Console.

A simple video showing some its feature is on youtube https://youtu.be/ycZjQUjz1Fk

Another video attempting to show the input lag is here: https://youtu.be/V7Pnba7Y12Y

Another video showing the Steel Battalion Controller support is here: https://youtu.be/g_eQlOcccg8

![alt text](https://github.com/Ryzee119/ogx360/blob/master/Images/image1.jpg?raw=true "ogx360-1")

## Features include:
* Support up to 4 Xbox Wireless Controllers with readily available [Arduino Pro Micro](https://www.sparkfun.com/products/12640) boards. I have linked Sparkfun but any cheaper clone will work the same.
* Full rumble support on all controllers when using Xbox 360 wireless controllers.
* Steel Battalion controller support on Player 1 with an Xbox 360 Chatpad using Xbox 360 Wireless Controllers.
* Basically instant boot up time so you can use controllers straight away from power on.
* Low level programming with minimal input lag. Less than 4ms over an original controller. (See https://youtu.be/V7Pnba7Y12Y)
* Firmware can be updated over USB. No programming hardware is required. See [Firmware](https://github.com/Ryzee119/ogx360/tree/master/Firmware) for programming instructions.
* 100% USB Bus powered from the Original Xbox controller ports. No requirement for external power supplies.
* 100% compatibility?
* One ogx360 mutliple Xboxes. For a side by side system link setup, just plug the 2nd module into the 2nd Xbox and you can use one ogx360 to control two (up to 4) Xboxes wirelessly.


## Supported Controllers
* Supports 4 players with Genuine and Third Party Microsoft Xbox 360 Wireless Receivers.
* Wired Xbox 360 Controllers.
* 8bitdo Wireless Adaptor (See http://www.8bitdo.com/wireless-usb-adapter/ for supported controllers). One controller per adaptor.
* ChronusMAX (See https://cronusmax.com/ for supported devices). Ensure it is configured to appear as an Xbox 360 Controller. One controller per adaptor.
* Genuine Wired Xbox One Controllers.


## Steel Battalion controller emulation
You need a wireless Xbox 360 controller with a chatpad connection. You can change what the controller appears to the console at anytime, so you can use the 'Duke Controller' to boot into the game, then change to the 'Steel Battalion controller' whilst in game. 

Button mapping is below. (High Res image available [here](https://i.imgur.com/12SawzC.png)).

![alt text](https://github.com/Ryzee119/ogx360/blob/master/Images/steelbattalion.png?raw=true "sbmapping")


## Folder structure
See the respective folder for open source licensing.

| Folder | Description |
| --- | --- |
| [`Hardware`](https://github.com/Ryzee119/ogx360/tree/master/Hardware) | This folder contains the schematic and PCB layout files. These were created using [Autodesk Eagle 9.2.2.](https://www.autodesk.com/products/eagle/overview)|
| [`Firmware`](https://github.com/Ryzee119/ogx360/tree/master/Firmware) | This folder contains the source code. The source code is based on the [USB Host Shield Library](https://github.com/felis/USB_Host_Shield_2.0) for Arduino, and [LUFA USB AVR Framework](http://www.fourwalledcubicle.com/LUFA.php). The code was integrated and compiled using [Atmel Studio 7.0](https://www.microchip.com/mplab/avr-support/atmel-studio-7).| 
| [`Cases`](https://github.com/Ryzee119/ogx360/tree/master/Cases) | This folder contains some cases built by myself and the community. If you have a design let me know and I will add it!| 
| [`Images`](https://github.com/Ryzee119/ogx360/tree/master/Images) | Just the images used throughout this repository.| 

## I have a PCB and want to add more modules
You can support up to 4 players with the addition of low cost Arduino Modules such as [this](https://www.aliexpress.com/item/New-Pro-Micro-for-arduino-ATmega32U4-5V-16MHz-Module-with-2-row-pin-header-For-Leonardo/32768308647.html).

This requires some basic soldering to solder the module into the respective port on the ogx360. Once the module is in place, you must program the correct firmware onto the device as per the instructions in [`Firmware`](https://github.com/Ryzee119/ogx360/tree/master/Firmware).
For Player 2, 3 and 4 modules you will want to use the slave hex file. Finally, connect a corresponding cable between the Arduino Module and the OG Xbox Controller Port!

## Connection to your Xbox
![connection](https://i.imgur.com/oz59WBT.jpg)
![connection1](https://github.com/Ryzee119/ogx360/blob/master/Images/image2.jpg?raw=true "ogx360-1")
![prototype](https://github.com/Ryzee119/ogx360/blob/master/Images/image3.jpg?raw=true "ogx360-1")

By Ryzee119
