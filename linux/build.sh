#!/usr/bin/bash

cc -std=gnu99 -g -D_DEBUG -Wall -Wextra -Wno-unused-parameter src/main.c src/ai.c src/animation.c src/audio_sdl.c src/characters.c src/collision.c src/console.c src/text.c src/font_sdl.c src/gamepad_sdl.c src/menus_desktop.c src/menu_actions.c src/input.c src/defaults_linux.c src/defaults_file.c src/network.c src/scenery.c src/texture.c src/texture_sdl.c src/thread_posix.c src/quit_sdl.c src/time_sdl.c src/window_sdl.c src/keyboard_sdl.c src/app_sdl.c src/mt_random.c src/weapon.c src/renderer.c src/renderer_gl.c src/renderer_projection.c `sdl2-config --libs` `sdl2-config --cflags` -lm -lX11 -lSDL2_mixer -lSDL2_ttf -lGLEW -lGL -lpthread -o skycheckers

cp gamecontrollerdb.txt Data/

if [ -f "Data/Shaders" ]; then
	rm -rf Data/Shaders
fi

cp -R linux/Shaders Data/Shaders
