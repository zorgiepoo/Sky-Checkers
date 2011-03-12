all:
	gcc main.c ai.c animation.c audio.c characters.c collision.c console.c font.c game_menus.c input.c linux.c menu.c network.c scenery.c utilities.c weapon.c libSDL_image.a libogg.a libvorbis.a libSDL_mixer.a libSDL_ttf.a libglut.a `freetype-config --cflags --libs` `sdl-config --libs` -lSDL -lGL -lGLU -o sc-bin
