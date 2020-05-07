#!/bin/sh
#flatpak-builder flatpak-build net.zgcoder.skycheckers.json --force-clean
#flatpak-builder --run build-dir net.zgcoder.skycheckers.json skycheckers

flatpak remove --user -y net.zgcoder.skycheckers
flatpak-builder --gpg-sign=$GPGKEY --repo=sc-repo --force-clean flatpak-build net.zgcoder.skycheckers.json
flatpak --user remote-add --if-not-exists sc-repo sc-repo
flatpak --user install sc-repo net.zgcoder.skycheckers

