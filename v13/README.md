# Wasca version 1.3, by cafe-alpha, Autumn 2018

## Differences with v1.2
 - Communication with PC via FTDI
 - Expansion RAM emulation ... not working well
 - Stub of SPI communication with STM32 ... chip not available in this board

## About each directories
 - board : PCB sources
 - wasca_10m08scv4k_no_spi_20190420 : "stable" version of the firmware with only a minimal set of functions working.
 - wasca_10m08sc_20191205_abus_divide : attempt to make expansion RAM working ... which breaks several things.
 - WascaLoader : test bench for use on Saturn side, which contains several functionalities to test expansion RAM.

## How to compile firmware
 1. Quartus version used : 15.1.2 Lite
 2. Rebuild Qsys sources
 3. Compile the project
 4. In software -> wasca_test9 folder, load mk.bat
 5. Type "rb" and enter to rebuild NIOS part of the firmware

## Pictures :
![image](https://github.com/cafe-alpha/wascafe/blob/master/v13/pictures/20181022_wasca_xrider.jpg)
![image](https://github.com/cafe-alpha/wascafe/blob/master/v13/pictures/20190405_wasca_almost_signature.jpg)
![image](https://github.com/cafe-alpha/wascafe/blob/master/v13/pictures/20191211_chuntest_8b.jpg)

More pictures and informations available on [my Saturn cartridges gallery](https://ppcenter.webou.net/satcart/gallery/#revw13).

