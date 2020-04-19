DESTDIR?=/

SOURCE=../src/main.c ../src/ai.c ../src/animation.c ../src/audio_sdl.c ../src/characters.c ../src/collision.c ../src/console.c ../src/text.c ../src/font_sdl.c ../src/gamepad_sdl.c ../src/menus_desktop.c ../src/menu_actions.c ../src/input.c ../src/defaults_linux.c ../src/defaults_file.c ../src/network.c ../src/scenery.c ../src/texture.c ../src/texture_sdl.c ../src/thread_posix.c ../src/quit_sdl.c ../src/time_sdl.c ../src/window_sdl.c ../src/keyboard_sdl.c ../src/app_sdl.c ../src/mt_random.c ../src/weapon.c ../src/renderer.c ../src/renderer_gl.c ../src/renderer_projection.c

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
	cp -R Data $(DESTDIR)/usr/share/skycheckers/Data
	mkdir -p $(DESTDIR)/usr/share/icons/
	cp skycheckers.png $(DESTDIR)/usr/share/icons/skycheckers.png
	mkdir -p $(DESTDIR)/usr/share/applications
	cp skycheckers.desktop $(DESTDIR)/usr/share/applications/skycheckers.desktop

.PHONY: uninstall
uninstall:
	rm -f $(DESTDIR)/usr/bin/skycheckers
	rm -rf $(DESTDIR)/usr/share/skycheckers
	rm -f $(DESTDIR)/usr/share/icons/skycheckers.png
	rm -f $(DESTDIR)/usr/share/applications/skycheckers.desktop

