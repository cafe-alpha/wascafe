#!/bin/bash
#
# Upload design and NIOS program

nios2-configure-sof --cable USB-Blaster ../../output_files/wasca.sof && make download-elf
