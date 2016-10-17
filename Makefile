all:
	gcc -O2 -Wall -Wextra -Wno-unused-parameter src/main.c src/ai.c src/animation.c src/audio.c src/characters.c src/collision.c src/console.c src/font.c src/game_menus.c src/input.c src/linux.c src/menu.c src/network.c src/scenery.c src/utilities.c src/weapon.c `sdl-config --libs` -lm -lX11 -lSDL_mixer -lSDL_ttf -lSDL_image -lGL -lGLU -lglut -o skycheckers
