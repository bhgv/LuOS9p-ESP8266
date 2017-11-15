#!/bin/bash

platform=esp8266

export ROOT=$PWD/

echo $ROOT

source ./config/env
cd ./platform/$platform/bootloader
#make $2 -j4 -C main UARTPORT=/dev/ttyUSB0 
make 
