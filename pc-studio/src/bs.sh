#!/bin/sh

LUA="$DIR/bld"
INC="-I$LUA/include -I. -I$DIR/lua-5.3.3/src"
LIB="-L$LUA/lib -llua -lm -ldl -Wl,-E -lreadline"

#echo gcc -c -g -O2 -fPIC $INC *.c 
gcc -c -g -O2 -std=gnu99 -fPIC -Wall -Wextra -DLUA_COMPAT_5_2 -DLUA_USE_LINUX $INC *.c 
gcc -std=gnu99 -fPIC -o $LUA/prg *.o $LIB
