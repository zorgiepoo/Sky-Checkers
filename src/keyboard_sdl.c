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

#include "keyboard.h"

bool ZGTestReturnKeyCode(uint16_t keyCode)
{
	return (keyCode == SDL_SCANCODE_RETURN || keyCode == SDL_SCANCODE_RETURN || keyCode == SDL_SCANCODE_KP_ENTER);
}

bool ZGTestMetaModifier(uint64_t modifier)
{
#ifdef MAC_OS_X
	SDL_Keymod metaModifier = (KMOD_LGUI | KMOD_RGUI);
#else
	SDL_Keymod metaModifier = (KMOD_LCTRL | KMOD_RCTRL);
#endif
	return (metaModifier & modifier) != 0;
}

const char *ZGGetKeyCodeName(uint16_t keyCode)
{
	return SDL_GetScancodeName((SDL_Scancode)keyCode);
}

char *ZGGetClipboardText(void)
{
	if (!SDL_HasClipboardText())
	{
		return NULL;
	}

	return SDL_GetClipboardText();
}

void ZGFreeClipboardText(char *clipboardText)
{
	SDL_free(clipboardText);
}
