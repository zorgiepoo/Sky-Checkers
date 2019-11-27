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

#pragma once

#include <stdbool.h>

#define MUSIC_VOLUME					32
#define MENU_SOUND_VOLUME				32
#define SHOOTING_SOUND_VOLUME			16
#define TILE_FALLING_SOUND_VOLUME		32
#define TILE_DIEING_STONE_SOUND_VOLUME	8
#define MAX_VOLUME						128

#define MENU_SOUND_CHANNEL 0
#define SHOOTING_SOUND_MIN_CHANNEL 1
#define SHOOTING_SOUND_MAX_CHANNEL 4
#define TILE_FALLING_SOUND_MIN_CHANNEL 5
#define TILE_FALLING_SOUND_MAX_CHANNEL 52
#define DIEING_STONE_SOUND_MIN_CHANNEL 53
#define DIEING_STONE_SOUND_MAX_CHANNEL 80
#define MAX_CHANNELS (DIEING_STONE_SOUND_MAX_CHANNEL + 1)

#define AUDIO_FORMAT_SAMPLE_RATE 22050
#define AUDIO_FORMAT_NUM_CHANNELS 2

void initAudio(void);

void playMainMenuMusic(bool paused);
void playGameMusic(bool paused);

void stopMusic(void);
void pauseMusic(void);
void unPauseMusic(void);

void playMenuSound(void);
void playShootingSound(int soundIndex);
void playTileFallingSound(void);
void playDieingStoneSound(void);
