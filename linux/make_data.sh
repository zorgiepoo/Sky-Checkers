#!/bin/sh

make precopy
cp -R icons Data/icons
rm -f Data/icons/sc_icon.bmp
tar -czvf sc-snap-pak-data.tgz Data

