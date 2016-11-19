Wasca version 1.2, by cafe-alpha, Autumn 2016

Specs :
 - MAX10 FPGA (10M04SC or higher)
 - 32MB SDRAM/DDR support
 - SD card interface

Features :
 - Cartridge backup memory ... TBD
 - SD/SDHC card access ... TBD
 - RAM expansion ... TBD

This version features several boards :
 1. board/wasca : cartridge PCB, made from PCB version 1.1.
 2. board/usbcart : mezzanine USB board, for optional use on cartridge.
 3. board/max10_board : MAX10 custom evaluation board. Basically, it's "board/wasca" without cartridge connector.

"board/wasca" was designed and tested first ... and didn't worked, probably because of my poor soldering skills.
Consequently, I designed "board/max10_board" in order to focus development on features that doesn't require a Saturn in order to be tested. When theses bases features will be developed and tested enough, I plan to design an "interface cartridge" to plug between Saturn cartridge connector and this evaluation board.

Pictures :
![image](https://github.com/cafe-alpha/wascafe/blob/master/v12/pictures/revw12a_front_solder.jpg)
![image](https://github.com/cafe-alpha/wascafe/blob/master/v12/pictures/revw12b_front_solder.jpg)
![image](https://github.com/cafe-alpha/wascafe/blob/master/v12/pictures/max10_board_front.png)
![image](https://github.com/cafe-alpha/wascafe/blob/master/v12/pictures/max10_board_rear.png)
![image](https://github.com/cafe-alpha/wascafe/blob/master/v12/pictures/revw12c_open.jpg)
