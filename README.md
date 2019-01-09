# ogx360 - Overview

The ogx360 is a small circuit board which allows you to use Wireless Xbox 360 Controllers via an Xbox 360 Wireless Receiver on the Original Xbox Console.

A simple video showing some its feature is on youtube https://youtu.be/ycZjQUjz1Fk
A another video attempting to show the input lag is here: https://youtu.be/V7Pnba7Y12Y

Any questions please email me at n360-usb@outlook.com.

As I'm getting ALOT of emails asking this, I am currently getting a small batch made which will be added to my etsy store https://www.etsy.com/au/shop/Ryzee119 when I have them available. I will put updates on my twitter page here https://twitter.com/Ryzee119. It's quite a small batch so if they sell out I'll get more. I just want to ensure no issues with the batch before getting a larger order.

![alt text](https://github.com/Ryzee119/ogx360/blob/master/Images/image1.jpg?raw=true "ogx360-1")

#### Features include:
* Support up to 4 Xbox Wireless Controllers using a modular design with readily available [Arduino Pro Micro](https://www.sparkfun.com/products/12640) boards. I have linked Sparkfun but any cheaper clone will work the same.
* Full Rumble Pak support on all controllers.
* Firmware can be updated over USB. No programming hardware is required. See [Firmware](https://github.com/Ryzee119/ogx360/tree/master/Firmware) for programming instructions.
* Low level programming with minimal input lag. Less than 4ms over than an original controller.
* 100% USB Bus powered from the Original Xbox controller ports. No requirement for external power supplies.
* Basically instant boot up time so you can use controllers straight away from power on.
* To connect to the OG Xbox you will need an Xbox to USB adaptor, and a short microUSB cable.
* I can't claim 100% compatibility; however, I haven't found a game that doesn't work. If you find one, let me know and I'll see if I can work it out.
* Supports Genuine Microsoft Xbox 360 Wireless Receivers (USB_VID 0x045E, USB_PID 0x0719) , Mad Catz Receivers (USB_VID 0x1BAD, USB_PID 0x0719) and Generic Third Party Wireless Receivers (USB_VID 0x045E, USB_PID 0x0291). **Wired controllers are not supported.**

#### Folder Structure
See the respective folder for open source licensing.

| Folder | Description |
| --- | --- |
| [`Hardware`](https://github.com/Ryzee119/ogx360/tree/master/Hardware) | This folder contains the schematic and PCB layout files. These were created using [Autodesk Eagle 9.2.2.](https://www.autodesk.com/products/eagle/overview)|
| [`Firmware`](https://github.com/Ryzee119/ogx360/tree/master/Firmware) | This folder contains the source code. The source code is based on the [USB Host Shield Library](https://github.com/felis/USB_Host_Shield_2.0) for Arduino, and [LUFA USB AVR Framework](http://www.fourwalledcubicle.com/LUFA.php). The code was integrated and compiled using [Atmel Studio 7.0](https://www.microchip.com/mplab/avr-support/atmel-studio-7).| 
| [`Images`](https://github.com/Ryzee119/ogx360/tree/master/Images) | Just the images used throughout this repository.| 

#### Further Info
The USB Host Controller is a MAX3421 and uses the [USB Host Shield Library](https://github.com/felis/USB_Host_Shield_2.0). This enumerates and gathers data from the Xbox 360 Wireless Receiver.
This data is then converted into the correct format for the Original Xbox and sent to the USB device controllers. The USB device controllers use Atmega32U4 chips based on the [Arduino Pro Micro](https://www.sparkfun.com/products/12640). Up to four Arduino Pro Micro's can be installed either directly soldered in or installed on female sockets to support up to four players. The USB device controllers have been configured to appear as Original Xbox Controllers and trick the console into thinking a real controller is connected.

![alt text](https://github.com/Ryzee119/ogx360/blob/master/Images/image2.jpg?raw=true "ogx360-1")
![alt text](https://github.com/Ryzee119/ogx360/blob/master/Images/image3.jpg?raw=true "ogx360-1")

Note that the pictures show a prototype version with two modules installed. There is some minor differences between this and the final version.

By Ryzee119
