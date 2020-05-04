DESTDIR?=/
USER_DIRNAME?=usr
INSTALL_DATA?=1
RELEASEOPTS?=

ifeq ($(INSTALL_DATA),1)
INSTALL_SC_DATA=true
else
INSTALL_SC_DATA=false
endif

FILES=main.c ai.c animation.c audio_sdl.c characters.c collision.c console.c text.c font_sdl.c gamepad_sdl.c menus_desktop.c menu_actions.c input.c defaults_linux.c defaults_file.c network.c scenery.c texture.c texture_sdl.c thread_posix.c quit_sdl.c time_sdl.c window_sdl.c keyboard_sdl.c app_sdl.c mt_random.c weapon.c renderer.c renderer_gl.c renderer_projection.c

SOURCE=$(addprefix ../src/, $(FILES))

WARNINGS=-Wall -Wextra -Wno-unused-parameter

LIBS=-lm -lX11 -lGL -lpthread -lGLEW `sdl2-config --cflags --libs` -lSDL2_mixer -lSDL2_ttf

CSTD=-std=gnu99

skycheckers: precopy
	cc $(CSTD) -O2 $(RELEASEOPTS) $(WARNINGS) $(SOURCE) $(LIBS) -o skycheckers

scdev: precopy
	cc $(CSTD) -g -D_DEBUG $(WARNINGS) $(SOURCE) $(LIBS) -o scdev

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
install: uninstall
	cp skycheckers $(DESTDIR)/$(USER_DIRNAME)/bin/skycheckers
	-$(INSTALL_SC_DATA) && mkdir -p $(DESTDIR)/$(USER_DIRNAME)/share/skycheckers
	-$(INSTALL_SC_DATA) && cp -R Data $(DESTDIR)/$(USER_DIRNAME)/share/skycheckers/
	-$(INSTALL_SC_DATA) && mkdir -p $(DESTDIR)/$(USER_DIRNAME)/share/icons/hicolor/512x512/apps/
	-$(INSTALL_SC_DATA) && cp icons/512x512/com.zgcoder.skycheckers.png $(DESTDIR)/$(USER_DIRNAME)/share/icons/hicolor/512x512/apps/
	-$(INSTALL_SC_DATA) && mkdir -p $(DESTDIR)/$(USER_DIRNAME)/share/icons/hicolor/128x128/apps/
	-$(INSTALL_SC_DATA) && cp icons/128x128/com.zgcoder.skycheckers.png $(DESTDIR)/$(USER_DIRNAME)/share/icons/hicolor/128x128/apps/
	-$(INSTALL_SC_DATA) && mkdir -p $(DESTDIR)/$(USER_DIRNAME)/share/icons/hicolor/64x64/apps/
	-$(INSTALL_SC_DATA) && cp icons/64x64/com.zgcoder.skycheckers.png $(DESTDIR)/$(USER_DIRNAME)/share/icons/hicolor/64x64/apps/
	mkdir -p $(DESTDIR)/$(USER_DIRNAME)/share/applications
	cp com.zgcoder.skycheckers.desktop $(DESTDIR)/$(USER_DIRNAME)/share/applications/
	mkdir -p $(DESTDIR)/$(USER_DIRNAME)/share/metainfo
	cp com.zgcoder.skycheckers.metainfo.xml $(DESTDIR)/$(USER_DIRNAME)/share/metainfo/
	-$(INSTALL_SC_DATA) && touch -a -c /$(USER_DIRNAME)/share/icons/hicolor
	-$(INSTALL_SC_DATA) && gtk-update-icon-cache $(DESTDIR)/$(USER_DIRNAME)/share/icons/hicolor/

.PHONY: uninstall
uninstall:
	rm -f $(DESTDIR)/$(USER_DIRNAME)/bin/skycheckers
	rm -rf $(DESTDIR)/$(USER_DIRNAME)/share/skycheckers
	rm -f $(DESTDIR)/$(USER_DIRNAME)/share/icons/hicolor/512x512/apps/com.zgcoder.skycheckers.png
	rm -f $(DESTDIR)/$(USER_DIRNAME)/share/icons/hicolor/128x128/apps/com.zgcoder.skycheckers.png
	rm -f $(DESTDIR)/$(USER_DIRNAME)/share/icons/hicolor/64x64/apps/com.zgcoder.skycheckers.png
	rm -f $(DESTDIR)/$(USER_DIRNAME)/share/applications/com.zgcoder.skycheckers.desktop
	rm -f $(DESTDIR)/$(USER_DIRNAME)/share/metainfo/com.zgcoder.skycheckers.metainfo.xml
	-$(INSTALL_SC_DATA) && touch -a -c /$(USER_DIRNAME)/share/icons/hicolor 2>&1 >/dev/null
	-$(INSTALL_SC_DATA) && gtk-update-icon-cache $(DESTDIR)/$(USER_DIRNAME)/share/icons/hicolor/ 2>&1 >/dev/null

