all:
	cc -Wall -Wextra -Wno-unused-parameter main.c ai.c animation.c audio.c characters.c collision.c console.c font.c game_menus.c input.c linux.c menu.c network.c scenery.c utilities.c weapon.c `sdl-config --libs` -lm -lX11 -lSDL_mixer -lSDL_ttf -lSDL_image -lGL -lGLU -lglut -o skycheckers
