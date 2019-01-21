/*
 * Copyright 2010 Mayur Pawashe
 * https://zgcoder.net
 
 * This file is part of skycheckers.
 * skycheckers is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 
 * skycheckers is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with skycheckers.  If not, see <http://www.gnu.org/licenses/>.
 */


#include "audio.h"
#include "utilities.h"

#define MUSIC_CHANNEL					1
#define MUSIC_VOLUME					64
#define MENU_SOUND_VOLUME				32
#define WHOOSH_SOUND_VOLUME				16
#define TILE_FALLING_SOUND_VOLUME		32
#define TILE_GRAY_STONE_SOUND_VOLUME	10

static Mix_Music *gMainMenuMusic = NULL;
static Mix_Music *gGameMusic = NULL;

SDL_bool initAudio(void)
{
	SDL_bool success = Mix_OpenAudio(MIX_DEFAULT_FREQUENCY, AUDIO_S16SYS, 2, 1024) == 0;
	if (success)
	{
		Mix_VolumeMusic(MUSIC_VOLUME);
	}
	
	return success;
}

void playMainMenuMusic(void)
{
	gMainMenuMusic = Mix_LoadMUS("Data/Audio/main_menu.ogg");
	if (gMainMenuMusic)
	{
		Mix_PlayMusic(gMainMenuMusic, -1);
	}
}

void playGameMusic(void)
{
	gGameMusic = Mix_LoadMUS("Data/Audio/fast-track.wav");
	if (gGameMusic)
	{
		Mix_PlayMusic(gGameMusic, -1);
	}
}

void stopMusic(void)
{
	Mix_HaltMusic();
	
	if (gMainMenuMusic)
	{
		Mix_FreeMusic(gMainMenuMusic);
		gMainMenuMusic = NULL;
	}
	
	if (gGameMusic)
	{
		Mix_FreeMusic(gGameMusic);
		gGameMusic = NULL;
	}
}

void pauseMusic(void)
{
	Mix_PauseMusic();
}

void unPauseMusic(void)
{
	Mix_ResumeMusic();
}

SDL_bool isPlayingGameMusic(void)
{
	return gGameMusic != NULL;
}

SDL_bool isPlayingMainMenuMusic(void)
{
	return gMainMenuMusic != NULL;
}

void playMenuSound(void)
{
	static Mix_Chunk *menuSoundChunk = NULL;
	if (!menuSoundChunk)
	{
		menuSoundChunk = Mix_LoadWAV("Data/Audio/sound6.wav");
		Mix_Volume(1, MENU_SOUND_VOLUME);
	}
	
	Mix_PlayChannel(1, menuSoundChunk, 0);
}

void playShootingSound(void)
{
	static Mix_Chunk *whooshChunk = NULL;
	if (!whooshChunk)
	{
		whooshChunk = Mix_LoadWAV("Data/Audio/whoosh.wav");
		Mix_Volume(1, WHOOSH_SOUND_VOLUME);
	}
	
	Mix_PlayChannel(1, whooshChunk, 0);
}

void playTileFallingSound(void)
{
	static Mix_Chunk *tileFallingChunk = NULL;
	if (!tileFallingChunk)
	{
		tileFallingChunk = Mix_LoadWAV("Data/Audio/object_falls.wav");
	}
	
	static int channel = 2;
	
	if (Mix_Playing(channel) == 0)
	{
		Mix_Volume(channel, TILE_FALLING_SOUND_VOLUME);
		Mix_PlayChannel(channel, tileFallingChunk, 0);
	}
	
	channel++;
	if (channel == 7)
	{
		channel = 2;
	}
}

void playGrayStoneColorSound(void)
{
	static Mix_Chunk *grayStoneChunk = NULL;
	if (!grayStoneChunk)
	{
		grayStoneChunk = Mix_LoadWAV("Data/Audio/flock_of_birds.wav");
	}
	
	static int channel = 2;
	
	if (Mix_Playing(channel) == 0)
	{
		Mix_Volume(channel, TILE_GRAY_STONE_SOUND_VOLUME);
		Mix_PlayChannel(channel, grayStoneChunk, 0);
	}
	
	channel++;
	if (channel == 7)
	{
		channel = 2;
	}
}

