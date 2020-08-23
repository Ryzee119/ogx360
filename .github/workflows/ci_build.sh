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

# List files to confirm it worked
ls -la *
