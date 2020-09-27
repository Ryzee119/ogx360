#!/bin/bash

# Fail on error
set -e

# Build the project
cd ./Firmware/ogx360_32u4
rm -rf build
mkdir build
cd build
cmake ..
make VERBOSE=1
avr-objcopy -j .text -j .data -O ihex ogx360_32u4_master.elf ogx360_32u4_master.hex
avr-objcopy -j .text -j .data -O ihex ogx360_32u4_master_steelbattalion.elf ogx360_32u4_master_steelbattalion.hex
avr-objcopy -j .text -j .data -O ihex ogx360_32u4_slave.elf ogx360_32u4_slave.hex
# List files to confirm it worked
ls -la *
