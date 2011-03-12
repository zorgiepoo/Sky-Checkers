/*
 * Coder: Mayur Pawashe (Zorg)
 * http://zorg.tejat.net
 */

#pragma once

#include "maincore.h"
#include "menu.h"

#define MAX_SERVER_ADDRESS_SIZE	50
extern char gServerAddressString[MAX_SERVER_ADDRESS_SIZE];
extern int gServerAddressStringIndex;

#define MAX_USER_NAME_SIZE	12
extern char gUserNameString[MAX_USER_NAME_SIZE];
extern int gUserNameStringIndex;

extern Menu gCharacterConfigureKeys[4][6];
extern Menu gJoyStickConfig[4][6];
extern Menu *gVideoOptionsMenu;

extern Menu *gConfigureLivesMenu;
extern Menu *gScreenResolutionVideoOptionMenu;
extern Menu *gRefreshRateVideoOptionMenu;

extern Menu *gAIModeOptionsMenu;
extern Menu *gAINetModeOptionsMenu;

extern Menu *gRedRoverPlayerOptionsMenu;
extern Menu *gGreenTreePlayerOptionsMenu;
extern Menu *gBlueLightningPlayerOptionsMenu;

extern SDL_bool gDrawArrowsForCharacterLivesFlag;
extern SDL_bool gDrawArrowsForScreenResolutionsFlag;
extern SDL_bool gDrawArrowsForRefreshRatesFlag;
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
