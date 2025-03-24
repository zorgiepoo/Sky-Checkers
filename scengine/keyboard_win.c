/*
 MIT License

 Copyright (c) 2024 Mayur Pawashe

 Permission is hereby granted, free of charge, to any person obtaining a copy
 of this software and associated documentation files (the "Software"), to deal
 in the Software without restriction, including without limitation the rights
 to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 copies of the Software, and to permit persons to whom the Software is
 furnished to do so, subject to the following conditions:

 The above copyright notice and this permission notice shall be included in all
 copies or substantial portions of the Software.

 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 SOFTWARE.
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
