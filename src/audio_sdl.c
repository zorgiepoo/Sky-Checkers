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
#include "platforms.h"
#include "sdl_include.h"

#include <stdio.h>

#if PLATFORM_WINDOWS
#include "SDL_mixer.h"
#elif PLATFORM_LINUX
#include <SDL2/SDL_mixer.h>
#endif

static bool gInitializedAudio = SDL_FALSE;

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

void initAudio(void)
{
	if (SDL_Init(SDL_INIT_AUDIO) < 0)
	{
		fprintf(stderr, "Couldn't initialize SDL audio: %s\n", SDL_GetError());
		return;
	}
	
	if (Mix_OpenAudio(AUDIO_FORMAT_SAMPLE_RATE, AUDIO_F32SYS, AUDIO_FORMAT_NUM_CHANNELS, 1024) != 0)
	{
		fprintf(stderr, "Mix_OpenAudio() failed: %s\n", Mix_GetError());
		return;
	}
	
	Mix_AllocateChannels(MAX_CHANNELS);
	
	gMenuSoundChunk = Mix_LoadWAV("Data/Audio/sound6.wav");
	gShootingSoundChunk = Mix_LoadWAV("Data/Audio/whoosh.wav");
	gTileFallingChunk = Mix_LoadWAV("Data/Audio/object_falls.wav");
	gDieingStoneChunk = Mix_LoadWAV("Data/Audio/dieing_stone.wav");
	
	Mix_VolumeMusic(MUSIC_VOLUME);
	Mix_Volume(MENU_SOUND_CHANNEL, MENU_SOUND_VOLUME);
	setVolume(SHOOTING_SOUND_VOLUME, SHOOTING_SOUND_MIN_CHANNEL, SHOOTING_SOUND_MAX_CHANNEL);
	setVolume(TILE_FALLING_SOUND_VOLUME, TILE_FALLING_SOUND_MIN_CHANNEL, TILE_FALLING_SOUND_MAX_CHANNEL);
	setVolume(TILE_DIEING_STONE_SOUND_VOLUME, DIEING_STONE_SOUND_MIN_CHANNEL, DIEING_STONE_SOUND_MAX_CHANNEL);
	
	gInitializedAudio = SDL_TRUE;
}

void playMainMenuMusic(bool paused)
{
	if (!gInitializedAudio) return;
	
	gMainMenuMusic = Mix_LoadMUS("Data/Audio/main_menu.wav");
	if (gMainMenuMusic)
	{
		Mix_PlayMusic(gMainMenuMusic, -1);
		Mix_ResumeMusic(); // necessary on CentOS
		if (paused)
		{
			pauseMusic();
		}
	}
}

void playGameMusic(bool paused)
{
	if (!gInitializedAudio) return;
	
	gGameMusic = Mix_LoadMUS("Data/Audio/fast-track.wav");
	if (gGameMusic)
	{
		Mix_PlayMusic(gGameMusic, -1);
		Mix_ResumeMusic(); // necessary on CentOS
		if (paused)
		{
			pauseMusic();
		}
	}
}

void stopMusic(void)
{
	if (!gInitializedAudio) return;
	
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
	if (!gInitializedAudio) return;
	
	Mix_PauseMusic();
}

void unPauseMusic(void)
{
	if (!gInitializedAudio) return;
	
	Mix_ResumeMusic();
}

void playMenuSound(void)
{
	if (!gInitializedAudio) return;
	
	if (gMenuSoundChunk != NULL)
	{
		Mix_PlayChannel(MENU_SOUND_CHANNEL, gMenuSoundChunk, 0);
	}
}

void playShootingSound(int soundIndex)
{
	if (!gInitializedAudio) return;
	
	if (gShootingSoundChunk != NULL)
	{
		int channel = SHOOTING_SOUND_MIN_CHANNEL + soundIndex;
		Mix_PlayChannel(channel, gShootingSoundChunk, 0);
	}
}

void playTileFallingSound(void)
{
	if (!gInitializedAudio) return;
	
	if (gTileFallingChunk != NULL)
	{
		static int currentSoundIndex = 0;
		
		int channel = currentSoundIndex + TILE_FALLING_SOUND_MIN_CHANNEL;
		
		Mix_PlayChannel(channel, gTileFallingChunk, 0);
		
		currentSoundIndex = (currentSoundIndex + 1) % (TILE_FALLING_SOUND_MAX_CHANNEL - TILE_FALLING_SOUND_MIN_CHANNEL + 1);
	}
}

void playDieingStoneSound(void)
{
	if (!gInitializedAudio) return;
	
	if (gDieingStoneChunk != NULL)
	{
		static int currentSoundIndex = 0;
		
		int channel = currentSoundIndex + DIEING_STONE_SOUND_MIN_CHANNEL;
		
		Mix_PlayChannel(channel, gDieingStoneChunk, 0);
		
		currentSoundIndex = (currentSoundIndex + 1) % (DIEING_STONE_SOUND_MIN_CHANNEL - DIEING_STONE_SOUND_MAX_CHANNEL + 1);
	}
}
