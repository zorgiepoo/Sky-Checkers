#!/bin/sh

rm -rf /tmp/sc-snap-pak-repo
rm -rf sc-snap-pak-repo
rm -f sc-snap-pak-repo.tgz
rm -f sc-linux-data.tgz
cp -R ../../Sky-Checkers /tmp/sc-snap-pak-repo
rm -f /tmp/sc-snap-pak-repo/linux/skycheckers*.txt
rm -rf /tmp/sc-snap-pak-repo/linux/icons
rm -rf /tmp/sc-snap-pak-repo/linux/flatpak-build
rm -rf /tmp/sc-snap-pak-repo/linux/.flatpak-builder
rm -rf /tmp/sc-snap-pak-repo/linux/Data
rm -rf /tmp/sc-snap-pak-repo/linux/sc-snap-pak-data
rm -f /tmp/sc-snap-pak-repo/linux/skycheckers*.snap
rm -rf /tmp/sc-snap-pak-repo/linux/sc-repo
rm -rf /tmp/sc-snap-pak-repo/linux/Shaders
rm -f /tmp/sc-snap-pak-repo/linux/*.sh
rm -f /tmp/sc-snap-pak-repo/linux/snapcraft.yaml
rm -f /tmp/sc-snap-pak-repo/linux/com.zgcoder.skycheckers.json
rm -f /tmp/sc-snap-pak-repo/linux/com.zgcoder.skycheckers.appdata.xml
rm -rf /tmp/sc-snap-pak-repo/mac
rm -rf /tmp/sc-snap-pak-repo/win
rm -rf /tmp/sc-snap-pak-repo/Data
rm -rf /tmp/sc-snap-pak-repo/.git
rm -f /tmp/sc-snap-pak-repo/.gitignore
rm -f /tmp/sc-snap-pak-repo/gamecontrollerdb.txt
rm -f /tmp/sc-snap-pak-repo/src/*.m
rm -f /tmp/sc-snap-pak-repo/src/defaults_win.c
rm -f /tmp/sc-snap-pak-repo/src/float_metal.h
rm -f /tmp/sc-snap-pak-repo/src/gamepad_gccontroller.h
rm -f /tmp/sc-snap-pak-repo/src/renderer_d3d11.cpp
rm -f /tmp/sc-snap-pak-repo/src/renderer_d3d11.h
rm -f /tmp/sc-snap-pak-repo/src/renderer_metal.h
rm -f /tmp/sc-snap-pak-repo/src/renderer_metal.m
rm -f /tmp/sc-snap-pak-repo/src/shaders.metal
rm -f /tmp/sc-snap-pak-repo/src/thread_sdl.c
rm -f /tmp/sc-snap-pak-repo/src/touch.h
mv /tmp/sc-snap-pak-repo .
tar -czvf sc-snap-pak-repo.tgz sc-snap-pak-repo
rm -rf sc-snap-pak-repo


