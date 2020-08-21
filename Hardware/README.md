# ogx360 - Hardware
This folder contains the schematic and PCB layout files and the PCB Gerber files. These were created using [Autodesk Eagle 9.2.2.](https://www.autodesk.com/products/eagle/overview)

I will preface by saying that the design contains *very* small and fine pitch surface mount components. The pictures below are misleading, they are really small. So if you attempt to make one yourself you should be quite proficient in soldering and have a good set of tools and equipment available.

# PCB
Download the [PCB Gerber](https://github.com/Ryzee119/ogx360/tree/master/Hardware/Gerbers) files from this repository, upload to an PCB ordering service.

Use the following PCB parameters:

*2 layers, 98x37mm, 1.6mm thick, HASL, 1oz copper, no castellated holes, any colour you want!*

**These will be UNPOPULATED boards!**

# Bill of Materials
Digikey Quick Cart: https://www.digikey.com.au/short/pbnnjz (Does not include non-digikey items, see 'other items' below for the remainder)
This also shows the bare minimum quantities, I would suggest you get a few spares in case you drop one of the tiny resistors etc. If some basic components are out of stock, find an alternate. Use your best judgement.

| Qty | Value/Model | Description | Marking | URL |
| --- | --- | --- | --- | --- |
| 1 | RED | LED RED 0805 | PWR | https://www.digikey.com.au/product-detail/en/150080RS75000/732-4984-1-ND |
| 5 | 100n | CAP CER 100NF 0402 | C1,C4,C5,C7,C8 | https://www.digikey.com.au/product-detail/en/885012205018/732-7513-1-ND |
| 1 | 10u | CAP ALUM 10UF 35V SMD | C6 | https://www.digikey.com.au/product-detail/en/kemet/EDK106M035A9BAA/399-14794-1-ND |
| 1 | 12MHz | CRYSTALSMD-5X3.2-4PAD | Q1 | https://www.digikey.com.au/product-detail/en/ECS-120-12-30B-AGM-TR/XC2676CT-ND |
| 2 | 18p | CAP CER 18PF 0402 | C2,C3 | https://www.digikey.com.au/product-detail/en/walsin-technology-corporation/0402N180J500CT/1292-1266-1-ND |
| 5 | 1k | RES SMD 1K OHM 5% 1/16W 0402 | R3,R4, R5, R6, R7 | https://www.digikey.com.au/product-detail/en/yageo/RC0402JR-071KL/311-1.0KJRCT-ND |
| 2 | 33R | RES 33 OHM 1% 1/16W 0402 | R1,R2 | https://www.digikey.com.au/product-detail/en/yageo/RC0402FR-0733RL/311-33.0LRCT-ND |
| 1 | 8V/0.5A | PTC RESET FUSE 8V 500MA 1206 | F1 | https://www.digikey.com.au/product-detail/en/0ZCJ0050FF2G/507-1802-1-ND |
| 1 | 74HC4050 | IC BUFFER NON-INVERT 6V 16SOIC | U2 | https://www.digikey.com.au/product-detail/en/74HC4050D/74HC4050DCT-ND |
| 1 | MAX3421 | IC USB PERIPH/HOST CNTRL 32TQFP | U1 | https://www.digikey.com.au/product-detail/en/maxim-integrated/MAX3421EEHJ-T/MAX3421EEHJ-TCT-ND/4895460 |
| 1 | MIC5219 3.3V | IC REG LINEAR 3.3V 500MA SOT23-5 | U3 | https://www.digikey.com.au/product-detail/en/MIC5219-3.3YM5-TR/576-1281-1-ND |
| 3 | 1N4148 | DIODE GEN PURP 75V 150MA SOD523 | D1, D2, D3 | https://www.digikey.com.au/product-detail/en/1N4148X-TP/1N4148XTPMSCT-ND |
| 1 | AE11182-ND | CONN USB RTANG FMALE TYPE A SMD | J1 | https://www.digikey.com.au/product-detail/en/AU-Y1006/AE11182-ND |
| 1 | TL3305AF160QG | SWITCH TACTILE SPST-NO 50MA 12V | RESET | https://www.digikey.com.au/product-detail/en/TL3305AF160QG/EG5350CT-ND |

### Other Items
| Qty | Value/Model | Description | Marking | URL |
| --- | --- | --- | --- | --- |
| 4 | ARDUINO_LEONARDO_MICRO | Arduino Pro Micro Leonardo Board | U4,U5,U6,U7 | [Any clones](https://www.aliexpress.com/item/New-Pro-Micro-for-arduino-ATmega32U4-5V-16MHz-Module-with-2-row-pin-header-For-Leonardo/32768308647.html). Only one is compulsory. 2,3 and 4 will add support for Player 2,3, 4 respectively. Make sure you get the 5V/16Mhz variant. |
| 4 | Xbox to USB Adaptor | Original Xbox Controller to USB Female Adaptor | - | [Wherever you can find them](https://www.aliexpress.com/item/For-XBOX-USB-CABLE-Female-USB-to-Original-Xbox-Adapter-Cable-Convertion-Line/32952259456.html). You will also need a [short Micro USB cable or adaptor ](https://www.aliexpress.com/item/Newest-USB-to-Micro-5p-USB-Adapter-Converter-USB-2-0-A-Male-to-Micro/32848917453.html) to connect to the Arduino boards.  |
| 8 | 12x1 F SOCKET 0.1" | CONN HEADER FEM 12POS .1" | - | For example https://www.sparkfun.com/products/115. Optional. |


### Unpopulated PCB Preview
![alt text](https://github.com/Ryzee119/ogx360/blob/master/Images/pcb.jpg?raw=true "ogx360-2")

# Community
Any community mods and cool stuff you have done with the ogx360 and you want me to add here I will!
1. Anarchy42085 over at www.ogxbox.com has put together a detailed possible wiring diagram for an internal ogx360 installation. It includes fully switchable USB data lines to allow full use of the standard controller ports and even XERC2 integration. All this and other stuff is [available here.](https://drive.google.com/drive/folders/1RyqyB42XzIUHupb7ZMqJmVH6tJKyBwMU) 

By Ryzee119
