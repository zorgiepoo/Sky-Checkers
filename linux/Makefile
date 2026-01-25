# Copyright 2020 Mayur Pawashe
# https://zgcoder.net
# 
# This file is part of skycheckers.
# skycheckers is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
# 
# skycheckers is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
# 
# You should have received a copy of the GNU General Public License
# along with skycheckers.  If not, see <http://www.gnu.org/licenses/>.

DESTDIR?=/
USER_DIRNAME?=usr
INSTALL_DATA?=1
RELEASEOPTS?=

ifeq ($(INSTALL_DATA),1)
INSTALL_SC_DATA=true
else
INSTALL_SC_DATA=false
endif

FILES=main.c ai.c animation.c audio_sdl.c characters.c collision.c console.c menus_desktop.c menu_actions.c input.c network.c scenery.c weapon.c

FILES_ENGINE=text.c font_sdl.c gamepad_sdl.c defaults_linux.c defaults_file.c texture.c texture_sdl.c thread_posix.c quit_sdl.c time_sdl.c window_sdl.c keyboard_sdl.c app_sdl.c mt_random.c renderer.c renderer_gl.c renderer_projection.c

SOURCE=$(addprefix ../src/, $(FILES)) $(addprefix ../scengine/, $(FILES_ENGINE)) ../vendor/glad/src/gl.c

WARNINGS=-Wall -Wextra -Wno-unused-parameter -Wno-format-truncation -Wno-calloc-transposed-args

LIBS=-lm -lX11 -lGL -lpthread `pkg-config sdl3 --cflags --libs` -lSDL3_mixer -lSDL3_ttf

INCLUDE_SEARCH=-I ../src -I ../scengine -I ../vendor/glad/include

CSTD=-std=gnu99

skycheckers: precopy
	cc $(CSTD) -O2 $(RELEASEOPTS) $(WARNINGS) $(SOURCE) $(INCLUDE_SEARCH) $(LIBS) -o skycheckers

scdev: precopy
	cc $(CSTD) -g -D_DEBUG $(WARNINGS) $(SOURCE) $(INCLUDE_SEARCH) $(LIBS) -o scdev

.PHONY: precopy
precopy: clean
	-$(INSTALL_SC_DATA) && cp -R ../Data Data
	-$(INSTALL_SC_DATA) && cp icons/sc_icon.bmp Data/Textures/
	-$(INSTALL_SC_DATA) && cp ../gamecontrollerdb.txt Data/
	-$(INSTALL_SC_DATA) && cp -R Shaders Data/Shaders

.PHONY: clean
clean:
	rm -rf Data
	rm -f skycheckers
	rm -f scdev

.PHONY: install
install:
	mkdir -p $(DESTDIR)/$(USER_DIRNAME)/bin/
	cp skycheckers $(DESTDIR)/$(USER_DIRNAME)/bin/skycheckers
	-$(INSTALL_SC_DATA) && mkdir -p $(DESTDIR)/$(USER_DIRNAME)/share/skycheckers
	-$(INSTALL_SC_DATA) && cp -R Data $(DESTDIR)/$(USER_DIRNAME)/share/skycheckers/
	mkdir -p $(DESTDIR)/$(USER_DIRNAME)/share/icons/hicolor/512x512/apps/
	cp icons/512x512/net.zgcoder.skycheckers.png $(DESTDIR)/$(USER_DIRNAME)/share/icons/hicolor/512x512/apps/
	mkdir -p $(DESTDIR)/$(USER_DIRNAME)/share/icons/hicolor/128x128/apps/
	cp icons/128x128/net.zgcoder.skycheckers.png $(DESTDIR)/$(USER_DIRNAME)/share/icons/hicolor/128x128/apps/
	mkdir -p $(DESTDIR)/$(USER_DIRNAME)/share/icons/hicolor/64x64/apps/
	cp icons/64x64/net.zgcoder.skycheckers.png $(DESTDIR)/$(USER_DIRNAME)/share/icons/hicolor/64x64/apps/
	mkdir -p $(DESTDIR)/$(USER_DIRNAME)/share/applications
	cp net.zgcoder.skycheckers.desktop $(DESTDIR)/$(USER_DIRNAME)/share/applications/
	mkdir -p $(DESTDIR)/$(USER_DIRNAME)/share/metainfo
	cp net.zgcoder.skycheckers.metainfo.xml $(DESTDIR)/$(USER_DIRNAME)/share/metainfo
	-$(INSTALL_SC_DATA) && touch -a -c /$(USER_DIRNAME)/share/icons/hicolor
	-$(INSTALL_SC_DATA) && gtk-update-icon-cache $(DESTDIR)/$(USER_DIRNAME)/share/icons/hicolor/

.PHONY: uninstall
uninstall:
	rm -f $(DESTDIR)/$(USER_DIRNAME)/bin/skycheckers
	rm -rf $(DESTDIR)/$(USER_DIRNAME)/share/skycheckers
	rm -f $(DESTDIR)/$(USER_DIRNAME)/share/icons/hicolor/512x512/apps/net.zgcoder.skycheckers.png
	rm -f $(DESTDIR)/$(USER_DIRNAME)/share/icons/hicolor/128x128/apps/net.zgcoder.skycheckers.png
	rm -f $(DESTDIR)/$(USER_DIRNAME)/share/icons/hicolor/64x64/apps/net.zgcoder.skycheckers.png
	rm -f $(DESTDIR)/$(USER_DIRNAME)/share/applications/net.zgcoder.skycheckers.desktop
	rm -f $(DESTDIR)/$(USER_DIRNAME)/share/metainfo/net.zgcoder.skycheckers.metainfo.xml
	-$(INSTALL_SC_DATA) && touch -a -c /$(USER_DIRNAME)/share/icons/hicolor 2>&1 >/dev/null
	-$(INSTALL_SC_DATA) && gtk-update-icon-cache $(DESTDIR)/$(USER_DIRNAME)/share/icons/hicolor/ 2>&1 >/dev/null

