#!/bin/sh

make precopy
rm -rf sc-snap-pak-data
mkdir sc-snap-pak-data
mv Data sc-snap-pak-data
tar -czvf sc-snap-pak-data.tgz sc-snap-pak-data
rm -rf sc-snap-pak-data

