


# ogx360 - Overview

The ogx360 is a small circuit board which allows you to use Wireless Xbox 360 Controllers via an Xbox 360 Wireless Receiver on the Original Xbox Console.

A simple video showing some its feature is on youtube https://youtu.be/ycZjQUjz1Fk

Another video attempting to show the input lag is here: https://youtu.be/V7Pnba7Y12Y

Another video showing the Steel Battalion Controller support is here: https://youtu.be/g_eQlOcccg8

Any questions please email me at n360-usb@outlook.com.

You can buy pre-assembled boards from my etsy page: https://www.etsy.com/au/listing/672315615/ogx360-add-wireless-xbox-360-controller 

If sold out, I generally post updates on my twitter page https://twitter.com/Ryzee119

![alt text](https://github.com/Ryzee119/ogx360/blob/master/Images/image1.jpg?raw=true "ogx360-1")

## Features include:
* Support up to 4 Xbox Wireless Controllers using a modular design with readily available [Arduino Pro Micro](https://www.sparkfun.com/products/12640) boards. I have linked Sparkfun but any cheaper clone will work the same.
* Full rumble support on all controllers when using Xbox 360 wireless controllers
* Steel Battalion controller support on Player 1 with an Xbox 360 Chatpad using Xbox 360 Wireless Controllers.
* Basically instant boot up time so you can use controllers straight away from power on.
* Low level programming with minimal input lag. Less than 4ms over an original controller. (See https://youtu.be/V7Pnba7Y12Y)
* Firmware can be updated over USB. No programming hardware is required. See [Firmware](https://github.com/Ryzee119/ogx360/tree/master/Firmware) for programming instructions.
* 100% USB Bus powered from the Original Xbox controller ports. No requirement for external power supplies.
* I can't claim 100% compatibility; however, I haven't found a game that doesn't work. If you find one, let me know and I'll see if I can work it out.
* One ogx360 mutliple Xboxes. For a side by side system link setup, just plug the 2nd module into the 2nd Xbox and you can use one ogx360 to control two (up to 4) Xboxes wirelessly.


## Supported Controllers
* Supports 4 players with Genuine and Third Party Microsoft Xbox 360 Wireless Receivers (USB_VID 0x045E, USB_PID 0x0719),(USB_VID 0x1BAD, USB_PID 0x0719) or (USB_VID 0x045E, USB_PID 0x0291).
* The following devices work, but only support one player. They requires a USB 2.0 Hub to connect multiple controllers.
* Wired Xbox 360 Controllers
* 8bitdo Wireless Adaptor (See http://www.8bitdo.com/wireless-usb-adapter/ for supported controllers). One controller per adaptor.
* ChronusMAX (See https://cronusmax.com/ for supported devices). Ensure it is configured to appear as an Xbox 360 Controller. One controller per adaptor.
* Can support Genuine Wired Xbox One Controllers with alternative firmware. See [`Firmware`](https://github.com/Ryzee119/ogx360/tree/master/Firmware).
* Does it support ....? If it's not listed it's not supported. 8bitdo/ChronusMAX should cover 99% of cases.


## Steel Battalion controller emulation
Traditionally, to play Steel Battalion you must have the unique Steel Battalion Controller. These are quite expensive, hard to come by and take up alot of space so I imagine alot of us have never been able to play the game. It is not possible to play using a standard controller as the game won't accept any inputs.

I have gone to a lot of effort to write my own Xbox 360 chatpad driver and emulate the Steel Battalion Controller myself. Shout out to the guys over at www.ogxbox.com forums, and shoutout to @Kekule who provided a USB descriptor dump and logic analyser data. Also the Cxbx reloaded guys as their source code helped alot!

Now this obviously is not going to have the same immersion and intuitive gameplay as the real controller, but it is atleast an option for those who don't own the Steel Battalion controller. With a bit of practice it is very playable!

You can change what the controller appears to the console at anytime, so you can use the 'Duke Controller' to boot into the game, then change to the 'Steel Battalion controller' whilst in game. 

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

## Connection to Xbox
![connection](https://i.imgur.com/oz59WBT.jpg)
![connection1](https://github.com/Ryzee119/ogx360/blob/master/Images/image2.jpg?raw=true "ogx360-1")
![prototype](https://github.com/Ryzee119/ogx360/blob/master/Images/image3.jpg?raw=true "ogx360-1")

Note that the pictures show a prototype version with two modules installed. There is some minor differences between this and the final version.

By Ryzee119
