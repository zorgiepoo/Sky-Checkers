/*
 * Coder: Mayur Pawashe (Zorg)
 * http://zorg.tejat.net
 */

#pragma once

#include "maincore.h"

SDL_bool initAudio(void);

void playMainMenuMusic(void);
void playGameMusic(void);

void stopMusic(void);
void pauseMusic(void);
void unPauseMusic(void);
SDL_bool isPlayingGameMusic(void);
SDL_bool isPlayingMainMenuMusic(void);

void playMenuSound(void);
void playShootingSound(void);
void playTileFallingSound(void);
void playGrayStoneColorSound(void);
