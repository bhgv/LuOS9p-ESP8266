#!/bin/sh

make
strip ascii85.so
cp ascii85.so $DIR/bld/lib/lua/5.3
