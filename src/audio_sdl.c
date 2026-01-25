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

#include <SDL3/SDL.h>
#include <SDL3_mixer/SDL_mixer.h>

#include "audio.h"
#include "platforms.h"

#include <stdio.h>

// music is not accounted for in MAX_CHANNELS
#define MAX_AUDIO_TRACKS (MAX_CHANNELS + 1)

#define MUSIC_TRACK 0
#define SOUND_TRACK(channel) (channel + 1)

static bool gInitializedAudio = false;

static MIX_Audio *gMainMenuMusicAudio;
static MIX_Audio *gGameMusicAudio;
static MIX_Audio *gMenuSoundAudio;
static MIX_Audio *gShootingSoundAudio;
static MIX_Audio *gTileFallingAudio;
static MIX_Audio *gDieingStoneAudio;

static SDL_PropertiesID gMusicPropertiesID;

static MIX_Mixer *gMixer;
static MIX_Track *gTracks[MAX_AUDIO_TRACKS];

static void setVolume(int volume, int minTrack, int maxTrack)
{
	float gain = ((float)volume) / MAX_VOLUME;
	for (int track = minTrack; track <= maxTrack; track++)
	{
		MIX_Track *mixTrack = gTracks[track];
		if (mixTrack != NULL)
		{
			if (!MIX_SetTrackGain(mixTrack, gain))
			{
				fprintf(stderr, "Failed to set volume gain on track %d: %s\n", track, SDL_GetError());
			}
		}
	}
}

static void setAudio(MIX_Audio *audio, int minTrack, int maxTrack)
{
	for (int track = minTrack; track <= maxTrack; track++)
	{
		MIX_Track *mixTrack = gTracks[track];
		if (mixTrack != NULL)
		{
			if (!MIX_SetTrackAudio(mixTrack, audio))
			{
				fprintf(stderr, "Failed to set track audio for track %d: %s\n", track, SDL_GetError());
			}
		}
	}
}

static void playTrack(int track, bool loop)
{
	MIX_Track *mixTrack = gTracks[track];
	if (mixTrack == NULL)
	{
		return;
	}

	if (!MIX_PlayTrack(mixTrack, loop ? gMusicPropertiesID : 0))
	{
		fprintf(stderr, "failed to play track %d: %s\n", track, SDL_GetError());
	}
}

void initAudio(void)
{
	if (!MIX_Init())
	{
		fprintf(stderr, "Couldn't initialize SDL mixer audio: %s\n", SDL_GetError());
		return;
	}

	gMixer = MIX_CreateMixerDevice(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, NULL);
	if (gMixer == NULL)
	{
		fprintf(stderr, "Failed to create mixer device: %s\n", SDL_GetError());
		return;
	}
	
	for (int trackIndex = 0; trackIndex < MAX_AUDIO_TRACKS; trackIndex++)
	{
		MIX_Track *track = MIX_CreateTrack(gMixer);
		if (track == NULL)
		{
			fprintf(stderr, "Failed to create track %d: %s\n", trackIndex, SDL_GetError());
			break;
		}

		gTracks[trackIndex] = track;
	}

	gMenuSoundAudio = MIX_LoadAudio(gMixer, "Data/Audio/sound6.wav", true);
	if (gMenuSoundAudio == NULL)
	{
		fprintf(stderr, "Failed to load sound6.wav: %s\n", SDL_GetError());
	}
	else
	{
		setAudio(gMenuSoundAudio, SOUND_TRACK(MENU_SOUND_CHANNEL), SOUND_TRACK(MENU_SOUND_CHANNEL));
	}

	gShootingSoundAudio = MIX_LoadAudio(gMixer, "Data/Audio/whoosh.wav", true);
	if (gShootingSoundAudio == NULL)
	{
		fprintf(stderr, "Failed to load whoosh.wav: %s\n", SDL_GetError());
	}
	else
	{
		setAudio(gShootingSoundAudio, SOUND_TRACK(SHOOTING_SOUND_MIN_CHANNEL), SOUND_TRACK(SHOOTING_SOUND_MAX_CHANNEL));
	}

	gTileFallingAudio = MIX_LoadAudio(gMixer, "Data/Audio/object_falls.wav", true);
	if (gTileFallingAudio == NULL)
	{
		fprintf(stderr, "Failed to load object_falls.wav: %s\n", SDL_GetError());
	}
	else
	{
		setAudio(gTileFallingAudio, SOUND_TRACK(TILE_FALLING_SOUND_MIN_CHANNEL), SOUND_TRACK(TILE_FALLING_SOUND_MAX_CHANNEL));
	}

	gDieingStoneAudio = MIX_LoadAudio(gMixer, "Data/Audio/dieing_stone.wav", true);
	if (gDieingStoneAudio == NULL)
	{
		fprintf(stderr, "Failed to load dieing_stone.wav: %s\n", SDL_GetError());
	}
	else
	{
		setAudio(gDieingStoneAudio, SOUND_TRACK(DIEING_STONE_SOUND_MIN_CHANNEL), SOUND_TRACK(DIEING_STONE_SOUND_MAX_CHANNEL));
	}

	setVolume(MUSIC_VOLUME, MUSIC_TRACK, MUSIC_TRACK);
	setVolume(MENU_SOUND_VOLUME, SOUND_TRACK(MENU_SOUND_CHANNEL), SOUND_TRACK(MENU_SOUND_CHANNEL));

	setVolume(SHOOTING_SOUND_VOLUME, SOUND_TRACK(SHOOTING_SOUND_MIN_CHANNEL), SOUND_TRACK(SHOOTING_SOUND_MAX_CHANNEL));
	setVolume(TILE_FALLING_SOUND_VOLUME, SOUND_TRACK(TILE_FALLING_SOUND_MIN_CHANNEL), SOUND_TRACK(TILE_FALLING_SOUND_MAX_CHANNEL));
	setVolume(TILE_DIEING_STONE_SOUND_VOLUME, SOUND_TRACK(DIEING_STONE_SOUND_MIN_CHANNEL), SOUND_TRACK(DIEING_STONE_SOUND_MAX_CHANNEL));

	gMusicPropertiesID = SDL_CreateProperties();
	if (gMusicPropertiesID == 0)
	{
		fprintf(stderr, "Failed to create SDL properties: %s\n", SDL_GetError());
	}
	else
	{
		SDL_SetNumberProperty(gMusicPropertiesID, MIX_PROP_PLAY_LOOPS_NUMBER, -1);
	}
	
	gInitializedAudio = true;
}

void playMainMenuMusic(bool paused)
{
	if (!gInitializedAudio) return;
	
	static bool initializedMainMusic;
	if (!initializedMainMusic)
	{
		gMainMenuMusicAudio = MIX_LoadAudio(gMixer, "Data/Audio/main_menu.wav", true);
		if (gMainMenuMusicAudio == NULL)
		{
			fprintf(stderr, "Failed to load main_menu.wav: %s\n", SDL_GetError());
		}

		initializedMainMusic = true;
	}

	if (gMainMenuMusicAudio == NULL)
	{
		return;
	}

	setAudio(gMainMenuMusicAudio, MUSIC_TRACK, MUSIC_TRACK);

	playTrack(MUSIC_TRACK, true);

	if (paused)
	{
		pauseMusic();
	}
}

void playGameMusic(bool paused)
{
	if (!gInitializedAudio) return;
	
	static bool initializedGameMusic;
	if (!initializedGameMusic)
	{
		gGameMusicAudio = MIX_LoadAudio(gMixer, "Data/Audio/fast-track.wav", true);
		if (gMainMenuMusicAudio == NULL)
		{
			fprintf(stderr, "Failed to load fast-track.wav: %s\n", SDL_GetError());
		}

		initializedGameMusic = true;
	}

	if (gGameMusicAudio == NULL)
	{
		return;
	}

	setAudio(gGameMusicAudio, MUSIC_TRACK, MUSIC_TRACK);

	playTrack(MUSIC_TRACK, true);

	if (paused)
	{
		pauseMusic();
	}
}

void stopMusic(void)
{
	if (!gInitializedAudio) return;

	if (gTracks[MUSIC_TRACK] != NULL && !MIX_StopTrack(gTracks[MUSIC_TRACK], 0))
	{
		fprintf(stderr, "Failed to stop music track: %s\n", SDL_GetError());
	}
}

void pauseMusic(void)
{
	if (!gInitializedAudio || gTracks[MUSIC_TRACK] == NULL) return;

	if (!MIX_PauseTrack(gTracks[MUSIC_TRACK]))
	{
		fprintf(stderr, "Failed to pause music track: %s\n", SDL_GetError());
	}
}

void unPauseMusic(void)
{
	if (!gInitializedAudio) return;

	if (!MIX_ResumeTrack(gTracks[MUSIC_TRACK]))
	{
		fprintf(stderr, "Failed to resume music track: %s\n", SDL_GetError());
	}
}

void playMenuSound(void)
{
	if (!gInitializedAudio || gMenuSoundAudio == NULL) return;

	playTrack(SOUND_TRACK(MENU_SOUND_CHANNEL), false);
}

void playShootingSound(int soundIndex)
{
	if (!gInitializedAudio || gShootingSoundAudio == NULL) return;

	playTrack(SOUND_TRACK(SHOOTING_SOUND_MIN_CHANNEL + soundIndex), false);
}

void playTileFallingSound(void)
{
	if (!gInitializedAudio || gTileFallingAudio == NULL) return;

	static int currentSoundIndex;

	playTrack(SOUND_TRACK(TILE_FALLING_SOUND_MIN_CHANNEL + currentSoundIndex), false);

	currentSoundIndex = (currentSoundIndex + 1) % (TILE_FALLING_SOUND_MAX_CHANNEL - TILE_FALLING_SOUND_MIN_CHANNEL + 1);
}

void playDieingStoneSound(void)
{
	if (!gInitializedAudio || gDieingStoneAudio == NULL) return;

	static int currentSoundIndex = 0;

	playTrack(SOUND_TRACK(DIEING_STONE_SOUND_MIN_CHANNEL + currentSoundIndex), false);

	currentSoundIndex = (currentSoundIndex + 1) % (DIEING_STONE_SOUND_MIN_CHANNEL - DIEING_STONE_SOUND_MAX_CHANNEL + 1);
}
