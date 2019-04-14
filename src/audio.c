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

#define MUSIC_VOLUME					64
#define MENU_SOUND_VOLUME				32
#define SHOOTING_SOUND_VOLUME			16
#define TILE_FALLING_SOUND_VOLUME		32
#define TILE_DIEING_STONE_SOUND_VOLUME	10

#define MENU_SOUND_CHANNEL 0
#define SHOOTING_SOUND_MIN_CHANNEL 1
#define SHOOTING_SOUND_MAX_CHANNEL 4
#define TILE_FALLING_SOUND_MIN_CHANNEL 5
#define TILE_FALLING_SOUND_MAX_CHANNEL 52
#define DIEING_STONE_SOUND_MIN_CHANNEL 53
#define DIEING_STONE_SOUND_MAX_CHANNEL 80
#define MAX_CHANNELS (DIEING_STONE_SOUND_MAX_CHANNEL + 1)

static Mix_Music *gMainMenuMusic = NULL;
static Mix_Music *gGameMusic = NULL;
static Mix_Chunk *gMenuSoundChunk = NULL;
static Mix_Chunk *gShootingSoundChunk = NULL;
static Mix_Chunk *gTileFallingChunk = NULL;
static Mix_Chunk *gDieingStoneChunk = NULL;

static void setVolume(int volume, int minChannel, int maxChannel)
{
	for (int channel = minChannel; channel <= maxChannel; channel++)
	{
		Mix_Volume(channel, volume);
	}
}

SDL_bool initAudio(void)
{
	SDL_bool success = Mix_OpenAudio(MIX_DEFAULT_FREQUENCY, AUDIO_S16SYS, 2, 1024) == 0;
	if (success)
	{
		Mix_VolumeMusic(MUSIC_VOLUME);
		
		Mix_AllocateChannels(MAX_CHANNELS);
		
		gMenuSoundChunk = Mix_LoadWAV("Data/Audio/sound6.wav");
		Mix_Volume(MENU_SOUND_CHANNEL, MENU_SOUND_VOLUME);
		
		gShootingSoundChunk = Mix_LoadWAV("Data/Audio/whoosh.wav");
		setVolume(SHOOTING_SOUND_VOLUME, SHOOTING_SOUND_MIN_CHANNEL, SHOOTING_SOUND_MAX_CHANNEL);
		
		gTileFallingChunk = Mix_LoadWAV("Data/Audio/object_falls.wav");
		setVolume(TILE_FALLING_SOUND_VOLUME, TILE_FALLING_SOUND_MIN_CHANNEL, TILE_FALLING_SOUND_MAX_CHANNEL);
		
		gDieingStoneChunk = Mix_LoadWAV("Data/Audio/flock_of_birds.wav");
		setVolume(TILE_DIEING_STONE_SOUND_VOLUME, DIEING_STONE_SOUND_MIN_CHANNEL, DIEING_STONE_SOUND_MAX_CHANNEL);
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
	if (gMenuSoundChunk != NULL)
	{
		Mix_PlayChannel(MENU_SOUND_CHANNEL, gMenuSoundChunk, 0);
	}
}

static int availableChannel(int minChannel, int maxChannel)
{
	for (int channel = minChannel; channel <= maxChannel; channel++)
	{
		if (Mix_Playing(channel) == 0)
		{
			return channel;
		}
	}
	return -1;
}

void playShootingSound(void)
{
	if (gShootingSoundChunk != NULL)
	{
		int channel = availableChannel(SHOOTING_SOUND_MIN_CHANNEL, SHOOTING_SOUND_MAX_CHANNEL);
		if (channel != -1)
		{
			Mix_PlayChannel(channel, gShootingSoundChunk, 0);
		}
	}
}

void playTileFallingSound(void)
{
	if (gTileFallingChunk != NULL)
	{
		int channel = availableChannel(TILE_FALLING_SOUND_MIN_CHANNEL, TILE_FALLING_SOUND_MAX_CHANNEL);
		if (channel != -1)
		{
			Mix_PlayChannel(channel, gTileFallingChunk, 0);
		}
	}
}

void playDieingStoneSound(void)
{
	if (gDieingStoneChunk != NULL)
	{
		int channel = availableChannel(DIEING_STONE_SOUND_MIN_CHANNEL, DIEING_STONE_SOUND_MAX_CHANNEL);
		if (channel != -1)
		{
			Mix_PlayChannel(channel, gDieingStoneChunk, 0);
		}
	}
}
