# ogx360 - Firmware

This folder contains some **experimental** source code. This functions the same as the standard ogx360 PCB however adds support for wired Xbox 360 controllers and Wired Xbox One Controllers. **Wireless Xbox One Controllers are not supported**
 
You need a USB2.0 hub to connect multiple wired controllers to the host side of the ogx360 PCB. In this build I have disabled rumble on Player 2, Player 3 and Player 4 as a safety precaution as power inrush on the rumble motors is quite high and my little PCB doesn't like it much. **Therefore I would recommend an externally powered USB hub if using multiple controllers.**
 
This software is released under GNU General Public License v3 (GPL-3).

# Experimental?
* My testing is limited to the controllers that I actually own. Therefore Wired Xbox 360 Controllers are *untested* and only one Genuine Wired Xbox One controller has been tested, however it should work with multiple via a USB Hub.
* Currently it doesn't support mixing controller types, i.e you can't have wireless and wired Xbox 360 controllers at the same time. This could be fixed I think.
* It's a bit finicky when hot plugging controllers so would suggest connecting everything before boot or power cycling the PCB when connecting controllers. Generally connecting and disconnecting a few times seems to do the trick.
* Support for third party Wired Xbox Controllers is unknown and untested. They way or may not work depending on how closely they mimick a genuine and their PID/VID combinations.

# Compiling
* Same as the Compiling instructions here https://github.com/Ryzee119/ogx360/tree/master/Firmware

# Programming
* Same as the Programming instructions here https://github.com/Ryzee119/ogx360/tree/master/Firmware

By Ryzee119
