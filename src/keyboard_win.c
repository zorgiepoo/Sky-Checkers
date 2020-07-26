/*
* Copyright 2020 Mayur Pawashe
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
#include <Windows.h>

#include <stdio.h>

bool ZGTestReturnKeyCode(uint16_t keyCode)
{
	return (keyCode == VK_RETURN);
}

bool ZGTestMetaModifier(uint64_t modifier)
{
	return (modifier == VK_CONTROL);
}

const char* ZGGetKeyCodeName(uint16_t keyCode)
{
	switch (keyCode)
	{
	case ZG_KEYCODE_RIGHT:
		return "right";
	case ZG_KEYCODE_LEFT:
		return "left";
	case ZG_KEYCODE_DOWN:
		return "down";
	case ZG_KEYCODE_UP:
		return "up";
	case ZG_KEYCODE_SPACE:
		return "space";
	}

	static char mappingBuffer[2];

	UINT mapping = MapVirtualKeyA(keyCode, MAPVK_VK_TO_CHAR);
	if (mapping != 0)
	{
		ZeroMemory(&mappingBuffer, sizeof(mappingBuffer));

		char characterValue = (char)(mapping & 0x7FFF);
		mappingBuffer[0] = characterValue;

		return mappingBuffer;
	}

	return NULL;
}

char* ZGGetClipboardText(void)
{
	if (!OpenClipboard(NULL))
	{
		fprintf(stderr, "Error: failed to open clipboard: %d\n", GetLastError());
		return NULL;
	}

	HANDLE handle = GetClipboardData(CF_TEXT);
	if (handle == NULL)
	{
		fprintf(stderr, "Error: failed to get clipboard data: %d\n", GetLastError());

		CloseClipboard();
		return NULL;
	}

	size_t bufferSize = 128;
	char *buffer = calloc(bufferSize, sizeof(*buffer));

	char *text = GlobalLock(handle);
	strncpy(buffer, text, bufferSize - 1);

	GlobalUnlock(handle);

	CloseClipboard();
	return buffer;
}

void ZGFreeClipboardText(char* clipboardText)
{
	free(clipboardText);
}
