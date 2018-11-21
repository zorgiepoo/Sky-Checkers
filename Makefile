all:
	cc -std=gnu99 -O2 -Wall -Wextra -Wno-unused-parameter src/main.c src/ai.c src/animation.c src/audio.c src/characters.c src/collision.c src/console.c src/font.c src/game_menus.c src/input.c src/linux.c src/menu.c src/network.c src/scenery.c src/utilities.c src/weapon.c `sdl2-config --libs` `sdl2-config --cflags` -lm -lX11 -lSDL2_mixer -lSDL2_ttf -lSDL2_image -lGL -o skycheckers
