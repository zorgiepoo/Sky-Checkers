#!/bin/sh
#flatpak-builder flatpak-build net.zgcoder.skycheckers.json --force-clean
#flatpak-builder --run build-dir net.zgcoder.skycheckers.json skycheckers

#https://github.com/flathub/shared-modules
#git submodule add https://github.com/flathub/shared-modules.git

flatpak remove --user -y net.zgcoder.skycheckers
flatpak-builder --gpg-sign=$GPGKEY --repo=sc-repo --force-clean flatpak-build net.zgcoder.skycheckers.json
flatpak --user remote-add --if-not-exists sc-repo sc-repo
flatpak --user install sc-repo net.zgcoder.skycheckers

# Alternatively to test:
#flatpak remove --user -y net.zgcoder.skycheckers
#flatpak-builder --repo=sc-repo --force-clean flatpak-build net.zgcoder.skycheckers.json
#flatpak-builder --user --install --force-clean build-dir net.zgcoder.skycheckers.json
#flatpak run net.zgcoder.skycheckers
