#!/bin/sh

LUA_INCDIR=$PWD/../bld/include PREFIX=$PWD/../bld make
cp ./periphery.so ../bld/lib/lua/5.3

