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

#include "maincore.h"
#include "menu.h"

#define MAX_SERVER_ADDRESS_SIZE	512
extern char gServerAddressString[MAX_SERVER_ADDRESS_SIZE];
extern int gServerAddressStringIndex;

#define MAX_USER_NAME_SIZE	12
extern char gUserNameString[MAX_USER_NAME_SIZE];
extern int gUserNameStringIndex;

extern Menu gCharacterConfigureKeys[4][6];
extern Menu gJoyStickConfig[4][6];

extern Menu *gConfigureLivesMenu;
extern Menu *gScreenResolutionVideoOptionMenu;
extern Menu *gRefreshRateVideoOptionMenu;

extern Menu *gAIModeOptionsMenu;
extern Menu *gAINetModeOptionsMenu;

extern Menu *gRedRoverPlayerOptionsMenu;
extern Menu *gGreenTreePlayerOptionsMenu;
extern Menu *gBlueLightningPlayerOptionsMenu;

extern SDL_bool gDrawArrowsForCharacterLivesFlag;
extern SDL_bool gDrawArrowsForAIModeFlag;
extern SDL_bool gDrawArrowsForNumberOfNetHumansFlag;
extern SDL_bool gDrawArrowsForNetPlayerLivesFlag;

extern SDL_bool gNetworkAddressFieldIsActive;
extern SDL_bool gNetworkUserNameFieldIsActive;

void initMenus(void);

void writeNetworkAddressText(Uint8 text);
void writeNetworkUserNameText(Uint8 text);
void performNetworkAddressBackspace(void);
void performNetworkUserNameBackspace(void);
