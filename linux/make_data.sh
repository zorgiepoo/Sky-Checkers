#!/bin/sh

make precopy
rm -rf sc-snap-pak-data
mkdir sc-snap-pak-data
mv Data sc-snap-pak-data
cp -R icons sc-snap-pak-data
cp com.zgcoder.skycheckers.appdata.xml sc-snap-pak-data
rm -f sc-snap-pak-data/icons/sc_icon.bmp
tar -czvf sc-snap-pak-data.tgz sc-snap-pak-data
rm -rf sc-snap-pak-data

