#!/bin/sh
#flatpak-builder flatpak-build com.zgcoder.skycheckers.json --force-clean
#flatpak-builder --run build-dir com.zgcoder.skycheckers.json skycheckers

flatpak remove --user -y com.zgcoder.skycheckers
flatpak-builder --repo=sc-repo --force-clean flatpak-build com.zgcoder.skycheckers.json
flatpak --user remote-add --if-not-exists --no-gpg-verify sc-repo sc-repo
flatpak --user install sc-repo com.zgcoder.skycheckers

