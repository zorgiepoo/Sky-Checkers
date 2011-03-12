/*
 * Coder: Mayur Pawashe (Zorg)
 * http://zorg.tejat.net
 */

#pragma once

#include "maincore.h"

void initConsole(void);

void buildBlackBorderDisplayList(void);
void deleteBlackBorderDisplayList(void);

void drawConsole(void);

void writeConsoleText(Uint8 text);

SDL_bool performConsoleBackspace(void);

void drawConsoleText(void);

void executeConsoleCommand(void);

void clearConsole(void);
