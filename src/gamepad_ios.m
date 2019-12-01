/*
* Copyright 2019 Mayur Pawashe
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

#import "gamepad.h"
#import <Foundation/Foundation.h>

struct _GamepadManager
{
};

GamepadManager *initGamepadManager(const char *databasePath, GamepadCallback addedCallback, GamepadCallback removalCallback)
{
	GamepadManager *gamepadManager = calloc(1, sizeof(*gamepadManager));
	return gamepadManager;
}

GamepadEvent *pollGamepadEvents(GamepadManager *gamepadManager, const void *systemEvent, uint16_t *eventCount)
{
	if (eventCount != NULL)
	{
		*eventCount = 0;
	}
	return NULL;
}

const char *gamepadName(GamepadManager *gamepadManager, GamepadIndex index)
{
	return "";
}
