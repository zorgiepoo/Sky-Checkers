DESTDIR?=/

FILES=main.c ai.c animation.c audio_sdl.c characters.c collision.c console.c text.c font_sdl.c gamepad_sdl.c menus_desktop.c menu_actions.c input.c defaults_linux.c defaults_file.c network.c scenery.c texture.c texture_sdl.c thread_posix.c quit_sdl.c time_sdl.c window_sdl.c keyboard_sdl.c app_sdl.c mt_random.c weapon.c renderer.c renderer_gl.c renderer_projection.c

SOURCE=$(addprefix ../src/, $(FILES))

WARNINGS=-Wall -Wextra -Wno-unused-parameter

LIBS=-lm -lX11 -lGL -lpthread -lGLEW `sdl2-config --cflags --libs` -lSDL2_mixer -lSDL2_ttf

CSTD=-std=gnu99

skycheckers: precopy
	cc $(CSTD) -O2 $(WARNINGS) $(SOURCE) $(LIBS) -o skycheckers

scdev: precopy
	cc $(CSTD) -g -D_DEBUG $(WARNINGS) $(SOURCE) $(LIBS) -o scdev

.PHONY: precopy
precopy: clean
	cp -R ../Data Data
	cp icons/sc_icon.bmp Data/Textures/
	cp ../gamecontrollerdb.txt Data/
	cp -R Shaders Data/Shaders

.PHONY: clean
clean:
	rm -rf Data
	rm -f skycheckers
	rm -f scdev

.PHONY: install
install: uninstall
	cp skycheckers $(DESTDIR)/usr/bin/skycheckers
	mkdir -p $(DESTDIR)/usr/share/skycheckers
	cp -R Data $(DESTDIR)/usr/share/skycheckers/
	mkdir -p $(DESTDIR)/usr/share/icons/hicolor/512x512/apps/
	cp icons/512x512/com.zgcoder.skycheckers.png $(DESTDIR)/usr/share/icons/hicolor/512x512/apps/
	mkdir -p $(DESTDIR)/usr/share/applications
	cp com.zgcoder.skycheckers.desktop $(DESTDIR)/usr/share/applications/

.PHONY: uninstall
uninstall:
	rm -f $(DESTDIR)/usr/bin/skycheckers
	rm -rf $(DESTDIR)/usr/share/skycheckers
	rm -f $(DESTDIR)/usr/share/icons/hicolor/512x512/apps/com.zgcoder.skycheckers.png
	rm -f $(DESTDIR)/usr/share/applications/com.zgcoder.skycheckers.desktop

