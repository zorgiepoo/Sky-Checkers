all:
	cc -std=gnu99 -g -D_DEBUG -Wall -Wextra -Wno-unused-parameter src/main.c src/ai.c src/animation.c src/audio_sdl.c src/characters.c src/collision.c src/console.c src/text.c src/font_sdl.c src/gamepad_sdl.c src/game_menus.c src/input.c src/linux.c src/menu.c src/network.c src/scenery.c src/texture_sdl.c src/thread_posix.c src/quit_sdl.c src/time_sdl.c src/window_sdl.c src/keyboard_sdl.c src/app_sdl.c src/mt_random.c src/weapon.c src/renderer.c src/renderer_gl.c src/renderer_projection.c `sdl2-config --libs` `sdl2-config --cflags` -lm -lX11 -lSDL2_mixer -lSDL2_ttf -lGLEW -lGL -o skycheckers
