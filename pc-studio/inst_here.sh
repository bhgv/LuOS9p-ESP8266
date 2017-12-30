#!/bin/sh

export DIR=$PWD

echo "  --> fonts"
mkdir ~/.fonts
ln -sf $DIR/bld/share/lua/5.3/tek/ui/font ~/.fonts

cd $DIR/bld

