# ogx360 - Overview
![CI](https://github.com/Ryzee119/ogx360/workflows/CI/badge.svg)  

If you like my work please consider a small donation<br>
[![paypal](https://img.shields.io/badge/Donate-PayPal-green.svg)](https://www.paypal.com/cgi-bin/webscr?cmd=_donations&business=49HV7N8QH9KQ8&currency_code=AUD&source=url)<br>

The ogx360 is a small circuit board which allows you to use a range of modern USB controllers on the Original Xbox Console.

* A simple video showing some its feature is on youtube https://youtu.be/ycZjQUjz1Fk
* Another video attempting to show the input lag is here: https://youtu.be/V7Pnba7Y12Y
* Another video showing the Steel Battalion Controller support is here: https://youtu.be/g_eQlOcccg8

<img src="./Images/image4.jpg" alt="ogx360 banner" width="100%"/>  

## Features include:
* Full rumble support on all controllers.
* Steel Battalion controller support with an Xbox 360 Chatpad using Xbox 360 Wireless Controllers.
* Low level programming with minimal input lag. Less than 4ms over an original controller. (See https://youtu.be/V7Pnba7Y12Y)
* Firmware can be updated over USB. No programming hardware is required. See [Firmware](./Firmware).
* One ogx360 mutliple Xboxes, just plug the other Arduino modules into nearby OG Xbox consoles.
* Flip the axis on the right stick for those games missing axis inversion settings. (Hold the right stick in then D-PAD to invert that direction).

## Supported controllers
* Supports 4 players with Genuine and Third Party Microsoft Xbox 360 Wireless Receivers.
* Wired Xbox 360 Controllers.
* 8bitdo Wireless Bluetooth Adaptor (See http://www.8bitdo.com/wireless-usb-adapter/ for supported controllers). One controller per 8bitdo adaptor.
* ChronusMAX/ChronusZen (See https://cronusmax.com/ for supported devices). Ensure it is configured to appear as an Xbox 360 Controller. One controller per adaptor.
* Most/all? controllers that are xinput compatible.
* Genuine and PDP Wired Xbox One Controllers.
* Hyperkin Duke Xbox One Controller.
* **NOTE** - When connecting multiple USB bus powered devices, a USB2.0 hub is required. A externally powered hub is recommended.

## Steel Battalion controller emulation
* You need a wireless Xbox 360 controller with a chatpad peripheral.
* You can change what the controller appears to the console at anytime, so you can use the 'Duke Controller' to boot into the game, then change to the 'Steel Battalion controller' whilst in game. 
* Button mapping is below. (High Res image available [here](https://i.imgur.com/12SawzC.png)).

<img src="./Images/steelbattalion.png" alt="sbmapping"/>  

## Connection to your Xbox
<img src="./Images/connection.jpg" alt="connection diagram"/>  

<img src="./Images/image2.jpg" alt="ogx360 in use"/>  

By Ryzee119
