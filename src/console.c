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

/* INSTRUCTIONS ON HOW TO USE CONSOLE
 * The console can be invoked by the tilde key ( ~ ).
 * To execute commands you hit RETURN on your keyboard.
 * 
 * Syntax for get commands:
 * get(arg) object.property
 * Only specifiy (arg) if you need to. You'll only need to if you want to get a tile's value.
 * example 1: get redRover.x // where redRover is the object and x is the property.
 * example 2: get redRover.weapon.z // note that objects can contain more objects. weapon is an object owned by redRover object in this case.
 * example 3: get(3) tile.x // The arg 3 specifies that the user wants the third tile's x property.
 *
 * Syntax for set commands:
 * object(arg).property value
 * Only specifiy (arg) if you need to. You'll only need to if you want to set a tile's value.
 * example 1: redRover.x 4.3 // sets redRover's object's property x to 4.3
 * example 2: redRover.weapon.z 8.3 // note that objects can contain more objects. weapon is an object owned by redRover object in this case. 
 * example 3: tile(5).z -20.0 // The arg 5 specifies that the user wants to set the fifth tile's z property to the value of -20.0
 * example 4: fps // notice that this command doesn't require any value to be inputted
 *
 * Also note that no commands contain capital letters and no objects or properties contain spaces, however,
 * they may contain underscores. For example, the tile object has a property called recovery_timer.
 */

#include "console.h"
#include "animation.h"
#include "characters.h"
#include "scenery.h"
#include "network.h"
#include "utilities.h"

// "scc~: " length == 6
static const unsigned int MIN_CONSOLE_STRING_LENGTH = 6;
static char gConsoleString[100];
static unsigned int gConsoleStringIndex;

static GLfloat getConsoleValue(void);
static GLfloat setConsoleValue(SDL_bool *errorFlag);

void initConsole(void)
{
	// syntax:
	// scc~: <message>
	gConsoleString[0] = 's';
	gConsoleString[1] = 'c';
	gConsoleString[2] = 'c';
	gConsoleString[3] = '~';
	gConsoleString[4] = ':';
	gConsoleString[5] = ' ';
	gConsoleString[6] = '\0';
	
	gConsoleStringIndex = MIN_CONSOLE_STRING_LENGTH;
}

void drawConsole(void)
{
	glTranslatef(0.0f, 9.0f, -25.0f);
	glColor4f(0.0f, 0.0f, 0.0f, 0.4f);
	
	GLfloat vertices[] =
	{
		10.0f, 1.0f, 1.0f,
		10.0f, -1.0f, 1.0f,
		-10.0f, -1.0f, 1.0f,
		-10.0f, 1.0f, 1.0f,
	};
	
	glEnableClientState(GL_VERTEX_ARRAY);
	
	glVertexPointer(3, GL_FLOAT, 0, vertices);
	glDrawArrays(GL_QUADS, 0, 4);
	
	glDisableClientState(GL_VERTEX_ARRAY);
	
	glLoadIdentity();
}

void writeConsoleText(Uint8 text)
{
	if (gConsoleStringIndex < 99)
	{
		gConsoleString[gConsoleStringIndex] = text;
		gConsoleString[gConsoleStringIndex + 1] = '\0';
		
		gConsoleStringIndex++;
	}
}

// returns true if it could perform a backspace
// otherwise, it returns false.
SDL_bool performConsoleBackspace(void)
{	
	if (gConsoleStringIndex > MIN_CONSOLE_STRING_LENGTH)
	{
		gConsoleStringIndex--;
		gConsoleString[gConsoleStringIndex] = '\0';
		
		return SDL_TRUE;
	}
	
	return SDL_FALSE;
}

void drawConsoleText(void)
{
	int i;
	int len = strlen(gConsoleString);
	
	glColor4f(1.0f, 0.0f, 0.0f, 0.7f);
	glRasterPos3f(-10.0f, 9.3f, -25.0f);
	
	for (i = 0; i <= len; i++)
	{
		glutBitmapCharacter(GLUT_BITMAP_9_BY_15, gConsoleString[i]);
	}
}

void executeConsoleCommand(void)
{	
	// invalid command
	if (!gConsoleString[MIN_CONSOLE_STRING_LENGTH] || gConsoleString[MIN_CONSOLE_STRING_LENGTH] == ' ' || !gConsoleString[MIN_CONSOLE_STRING_LENGTH + 1] || gConsoleString[MIN_CONSOLE_STRING_LENGTH + 1] == ' ')
		return;
	
	// get command
	if (gConsoleString[MIN_CONSOLE_STRING_LENGTH] == 'g' && gConsoleString[MIN_CONSOLE_STRING_LENGTH + 1] == 'e')
		zgPrint("(scc) get: %f", getConsoleValue());
	
	// set command
	else
	{
		SDL_bool errorFlag;
		GLfloat value = setConsoleValue(&errorFlag);
		if (!errorFlag)
		{
			zgPrint("(scc) set: %f", value);
		}
		else
		{
			zgPrint("(scc) set: error!");
		}
	}
}

static GLfloat getConsoleValue(void)
{
	// syntax:
	// scc~: get(arg) object.property
	
	GLfloat value = 0.0;
	char result[10];
	int resultLen;
	int i;
	
	// get() ...
	if (gConsoleString[MIN_CONSOLE_STRING_LENGTH + 3] == '(')
	{
		// we know that the user requests an arguement.. which must mean that the user wants a tile value.
		
		int arg_val;
		int stringIndexCount = 7;
		char *num = &gConsoleString[MIN_CONSOLE_STRING_LENGTH + 4];
		char *partialConsoleString;
		
		arg_val = atoi(num);
		
		// only valid arg values are 1 - 64 (for tiles)
		if (arg_val > 64 || arg_val <= 0)
			return value;
		
		// if it's two digits, increment stringIndexCount
		if (arg_val > 9)
			stringIndexCount++;
		
		partialConsoleString = &gConsoleString[MIN_CONSOLE_STRING_LENGTH + stringIndexCount];
		
		if (strcmp(partialConsoleString, "tile.x") == 0)
			value = gTiles[arg_val].x;
		else if (strcmp(partialConsoleString, "tile.y") == 0)
			value = gTiles[arg_val].y;
		else if (strcmp(partialConsoleString, "tile.z") == 0)
			value = gTiles[arg_val].z;
		else if (strcmp(partialConsoleString, "tile.red") == 0)
			value = gTiles[arg_val].red;
		else if (strcmp(partialConsoleString, "tile.green") == 0)
			value = gTiles[arg_val].green;
		else if (strcmp(partialConsoleString, "tile.blue") == 0)
			value = gTiles[arg_val].blue;
		else if (strcmp(partialConsoleString, "tile.recovery_timer") == 0)
			value = (GLfloat)gTiles[arg_val].recovery_timer;
		else if (strcmp(partialConsoleString, "tile.state") == 0)
			value = (GLfloat)gTiles[arg_val].state;
		else if (strcmp(partialConsoleString, "tile.is_dead") == 0)
			value = (GLfloat)gTiles[arg_val].isDead;
	}
	
	// get calls with no arguements
	
	// redRover variables.
	if (strcmp(gConsoleString, "scc~: get redRover.x") == 0)
		value = gRedRover.x;
	else if (strcmp(gConsoleString, "scc~: get redRover.y") == 0)
		value = gRedRover.y;
	else if (strcmp(gConsoleString, "scc~: get redRover.z") == 0)
		value = gRedRover.z;
	else if (strcmp(gConsoleString, "scc~: get redRover.xrot") == 0)
		value = gRedRover.xRot;
	else if (strcmp(gConsoleString, "scc~: get redRover.yrot") == 0)
		value = gRedRover.yRot;
	else if (strcmp(gConsoleString, "scc~: get redRover.zrot") == 0)
		value = gRedRover.zRot;
	else if (strcmp(gConsoleString, "scc~: get redRover.direction") == 0)
		value = (GLfloat)gRedRover.direction;
	else if (strcmp(gConsoleString, "scc~: get redRover.state") == 0)
		value = (GLfloat)gRedRover.state;
	else if (strcmp(gConsoleString, "scc~: get redRover.recovery_timer") == 0)
		value = (GLfloat)gRedRover.recovery_timer;
	else if (strcmp(gConsoleString, "scc~: get redRover.animation_timer") == 0)
		value = (GLfloat)gRedRover.animation_timer;
	else if (strcmp(gConsoleString, "scc~: get redRover.weapon.x") == 0)
		value = gRedRover.weap->x;
	else if (strcmp(gConsoleString, "scc~: get redRover.weapon.y") == 0)
		value = gRedRover.weap->y;
	else if (strcmp(gConsoleString, "scc~: get redRover.weapon.z") == 0)
		value = gRedRover.weap->z;
	else if (strcmp(gConsoleString, "scc~: get redRover.weapon.direction") == 0)
		value = (GLfloat)gRedRover.weap->direction;
	else if (strcmp(gConsoleString, "scc~: get redRover.weapon.drawing_state") == 0)
		value = (GLfloat)gRedRover.weap->drawingState;
	else if (strcmp(gConsoleString, "scc~: get redRover.weapon.animation_state") == 0)
		value = (GLfloat)gRedRover.weap->animationState;
	else if (strcmp(gConsoleString, "scc~: get redRover.weapon.red") == 0)
		value = gRedRover.weap->red;
	else if (strcmp(gConsoleString, "scc~: get redRover.weapon.green") == 0)
		value = gRedRover.weap->green;
	else if (strcmp(gConsoleString, "scc~: get redRover.weapon.blue") == 0)
		value = gRedRover.weap->blue;
	else if (strcmp(gConsoleString, "scc~: get redRover.time_alive") == 0)
		value = (GLfloat)gRedRover.time_alive;
	
	// greenTree variables.
	if (strcmp(gConsoleString, "scc~: get greenTree.x") == 0)
		value = gGreenTree.x;
	else if (strcmp(gConsoleString, "scc~: get greenTree.y") == 0)
		value = gGreenTree.y;
	else if (strcmp(gConsoleString, "scc~: get greenTree.z") == 0)
		value = gGreenTree.z;
	else if (strcmp(gConsoleString, "scc~: get greenTree.xrot") == 0)
		value = gGreenTree.xRot;
	else if (strcmp(gConsoleString, "scc~: get greenTree.yrot") == 0)
		value = gGreenTree.yRot;
	else if (strcmp(gConsoleString, "scc~: get greenTree.zrot") == 0)
		value = gGreenTree.zRot;
	else if (strcmp(gConsoleString, "scc~: get greenTree.direction") == 0)
		value = (GLfloat)gGreenTree.direction;
	else if (strcmp(gConsoleString, "scc~: get greenTree.state") == 0)
		value = (GLfloat)gGreenTree.state;
	else if (strcmp(gConsoleString, "scc~: get greenTree.recovery_timer") == 0)
		value = (GLfloat)gGreenTree.recovery_timer;
	else if (strcmp(gConsoleString, "scc~: get greenTree.animation_timer") == 0)
		value = (GLfloat)gGreenTree.animation_timer;
	else if (strcmp(gConsoleString, "scc~: get greenTree.weapon.x") == 0)
		value = gGreenTree.weap->x;
	else if (strcmp(gConsoleString, "scc~: get greenTree.weapon.y") == 0)
		value = gGreenTree.weap->y;
	else if (strcmp(gConsoleString, "scc~: get greenTree.weapon.z") == 0)
		value = gGreenTree.weap->z;
	else if (strcmp(gConsoleString, "scc~: get greenTree.weapon.direction") == 0)
		value = (GLfloat)gGreenTree.weap->direction;
	else if (strcmp(gConsoleString, "scc~: get greenTree.weapon.drawing_state") == 0)
		value = (GLfloat)gGreenTree.weap->drawingState;
	else if (strcmp(gConsoleString, "scc~: get greenTree.weapon.animation_state") == 0)
		value = (GLfloat)gGreenTree.weap->animationState;
	else if (strcmp(gConsoleString, "scc~: get greenTree.weapon.red") == 0)
		value = gGreenTree.weap->red;
	else if (strcmp(gConsoleString, "scc~: get greenTree.weapon.green") == 0)
		value = gGreenTree.weap->green;
	else if (strcmp(gConsoleString, "scc~: get greenTree.weapon.blue") == 0)
		value = gGreenTree.weap->blue;
	else if (strcmp(gConsoleString, "scc~: get greenTree.time_alive") == 0)
		value = (GLfloat)gGreenTree.time_alive;
	
	// pinkBubbleGum variables.
	if (strcmp(gConsoleString, "scc~: get pinkBubbleGum.x") == 0)
		value = gPinkBubbleGum.x;
	else if (strcmp(gConsoleString, "scc~: get pinkBubbleGum.y") == 0)
		value = gPinkBubbleGum.y;
	else if (strcmp(gConsoleString, "scc~: get pinkBubbleGum.z") == 0)
		value = gPinkBubbleGum.z;
	else if (strcmp(gConsoleString, "scc~: get pinkBubbleGum.xrot") == 0)
		value = gPinkBubbleGum.xRot;
	else if (strcmp(gConsoleString, "scc~: get pinkBubbleGum.yrot") == 0)
		value = gPinkBubbleGum.yRot;
	else if (strcmp(gConsoleString, "scc~: get pinkBubbleGum.zrot") == 0)
		value = gPinkBubbleGum.zRot;
	else if (strcmp(gConsoleString, "scc~: get pinkBubbleGum.direction") == 0)
		value = (GLfloat)gPinkBubbleGum.direction;
	else if (strcmp(gConsoleString, "scc~: get pinkBubbleGum.state") == 0)
		value = (GLfloat)gPinkBubbleGum.state;
	else if (strcmp(gConsoleString, "scc~: get pinkBubbleGum.recovery_timer") == 0)
		value = (GLfloat)gPinkBubbleGum.recovery_timer;
	else if (strcmp(gConsoleString, "scc~: get pinkBubbleGum.animation_timer") == 0)
		value = (GLfloat)gPinkBubbleGum.animation_timer;
	else if (strcmp(gConsoleString, "scc~: get pinkBubbleGum.weapon.x") == 0)
		value = gPinkBubbleGum.weap->x;
	else if (strcmp(gConsoleString, "scc~: get pinkBubbleGum.weapon.y") == 0)
		value = gPinkBubbleGum.weap->y;
	else if (strcmp(gConsoleString, "scc~: get pinkBubbleGum.weapon.z") == 0)
		value = gPinkBubbleGum.weap->z;
	else if (strcmp(gConsoleString, "scc~: get pinkBubbleGum.weapon.direction") == 0)
		value = (GLfloat)gPinkBubbleGum.weap->direction;
	else if (strcmp(gConsoleString, "scc~: get pinkBubbleGum.weapon.drawing_state") == 0)
		value = (GLfloat)gPinkBubbleGum.weap->drawingState;
	else if (strcmp(gConsoleString, "scc~: get pinkBubbleGum.weapon.animation_state") == 0)
		value = (GLfloat)gPinkBubbleGum.weap->animationState;
	else if (strcmp(gConsoleString, "scc~: get pinkBubbleGum.weapon.red") == 0)
		value = gPinkBubbleGum.weap->red;
	else if (strcmp(gConsoleString, "scc~: get pinkBubbleGum.weapon.green") == 0)
		value = gPinkBubbleGum.weap->green;
	else if (strcmp(gConsoleString, "scc~: get pinkBubbleGum.weapon.blue") == 0)
		value = gPinkBubbleGum.weap->blue;
	else if (strcmp(gConsoleString, "scc~: get pinkBubbleGum.time_alive") == 0)
		value = (GLfloat)gPinkBubbleGum.time_alive;
	
	// blueLightning variables.
	if (strcmp(gConsoleString, "scc~: get blueLightning.x") == 0)
		value = gBlueLightning.x;
	else if (strcmp(gConsoleString, "scc~: get blueLightning.y") == 0)
		value = gBlueLightning.y;
	else if (strcmp(gConsoleString, "scc~: get blueLightning.z") == 0)
		value = gBlueLightning.z;
	else if (strcmp(gConsoleString, "scc~: get blueLightning.xrot") == 0)
		value = gBlueLightning.xRot;
	else if (strcmp(gConsoleString, "scc~: get blueLightning.yrot") == 0)
		value = gBlueLightning.yRot;
	else if (strcmp(gConsoleString, "scc~: get blueLightning.zrot") == 0)
		value = gBlueLightning.zRot;
	else if (strcmp(gConsoleString, "scc~: get blueLightning.direction") == 0)
		value = (GLfloat)gBlueLightning.direction;
	else if (strcmp(gConsoleString, "scc~: get blueLightning.state") == 0)
		value = (GLfloat)gBlueLightning.state;
	else if (strcmp(gConsoleString, "scc~: get blueLightning.recovery_timer") == 0)
		value = (GLfloat)gBlueLightning.recovery_timer;
	else if (strcmp(gConsoleString, "scc~: get blueLightning.animation_timer") == 0)
		value = (GLfloat)gBlueLightning.animation_timer;
	else if (strcmp(gConsoleString, "scc~: get blueLightning.weapon.x") == 0)
		value = gBlueLightning.weap->x;
	else if (strcmp(gConsoleString, "scc~: get blueLightning.weapon.y") == 0)
		value = gBlueLightning.weap->y;
	else if (strcmp(gConsoleString, "scc~: get blueLightning.weapon.z") == 0)
		value = gBlueLightning.weap->z;
	else if (strcmp(gConsoleString, "scc~: get blueLightning.weapon.direction") == 0)
		value = (GLfloat)gBlueLightning.weap->direction;
	else if (strcmp(gConsoleString, "scc~: get blueLightning.weapon.drawing_state") == 0)
		value = (GLfloat)gBlueLightning.weap->drawingState;
	else if (strcmp(gConsoleString, "scc~: get blueLightning.weapon.animation_state") == 0)
		value = (GLfloat)gBlueLightning.weap->animationState;
	else if (strcmp(gConsoleString, "scc~: get blueLightning.weapon.red") == 0)
		value = gBlueLightning.weap->red;
	else if (strcmp(gConsoleString, "scc~: get blueLightning.weapon.green") == 0)
		value = gBlueLightning.weap->green;
	else if (strcmp(gConsoleString, "scc~: get blueLightning.weapon.blue") == 0)
		value = gBlueLightning.weap->blue;
	else if (strcmp(gConsoleString, "scc~: get blueLightning.time_alive") == 0)
		value = (GLfloat)gBlueLightning.time_alive;
	
	else if (strcmp(gConsoleString, "scc~: get ai_mode") == 0)
		value = (GLfloat)gAIMode;
	
	// write text for precision up to 1 digit past the decimal point.
	// note that we return the real value with all of its digits.
	sprintf(result, "%0.2f", value);
	
	// insert a space
	writeConsoleText(' ');
	
	resultLen = (int)strlen(result);
	
	for (i = 0; i <= resultLen; i++)
	{
		writeConsoleText(result[i]);
	}
	
	return value;
}

static GLfloat setConsoleValue(SDL_bool *errorFlag)
{
	*errorFlag = SDL_FALSE;
	
	// syntax:
	// scc~: object(arg).property value
	
	char input[50];
	GLfloat value = 0;
	unsigned int i;
	unsigned int len = 0;
	SDL_bool valueExists = SDL_FALSE;
	
	// find the space and set len to that index.
	for (i = MIN_CONSOLE_STRING_LENGTH; !valueExists && i < strlen(gConsoleString); i++)
	{
		if (isspace(gConsoleString[i]))
		{
			valueExists = SDL_TRUE;
			len = i;
		}
	}
	
	if (len == gConsoleStringIndex - 1)
	{
		*errorFlag = SDL_TRUE;
		return 0.0;
	}
	
	if (valueExists)
	{
		// input is now the variable/storage name
		strncpy(input, gConsoleString, len);
		input[len] = '\0';
		
		char *num = &gConsoleString[len + 1];
		
		// value is the value the user wants to set for the variable name.
		value = (GLfloat)atof(num);
		
		if (value == 0.0 && num[0] != '0')
		{
			*errorFlag = SDL_TRUE;
			return value;
		}
		valueExists = SDL_TRUE;
	}
	else
	{
		strcpy(input, gConsoleString);
		valueExists = SDL_FALSE;
		
		if (strcmp(input, "scc~: game_reset") != 0 && strcmp(input, "scc~: fps") != 0)
		{
			*errorFlag = SDL_TRUE;
			return 0.0;
		}
	}
	
	if ((!gNetworkConnection || gNetworkConnection->type != NETWORK_CLIENT_TYPE))
	{
		if (gConsoleString[MIN_CONSOLE_STRING_LENGTH + 4] == '(')
		{
			// the user is telling us an arguement, which means it wants to set a tile property value.
			int arg_val;
			int stringIndexCount = 7;
			char *number = &gConsoleString[MIN_CONSOLE_STRING_LENGTH + 5];
			char partialConsoleString[100];
			char *property;
			
			arg_val = atoi(number);
			
			if (arg_val > 64)
				return SDL_TRUE;
			
			if (arg_val > 9)
				stringIndexCount++;
			
			strcpy(partialConsoleString, gConsoleString);
			partialConsoleString[len] = '\0';
			
			property = (char *)&partialConsoleString;
			property += MIN_CONSOLE_STRING_LENGTH + stringIndexCount;
			
			if (strcmp(property, ".x") == 0)
				gTiles[arg_val].x = value;
			else if (strcmp(property, ".y") == 0)
				gTiles[arg_val].y = value;
			else if (strcmp(property, ".z") == 0)
				gTiles[arg_val].z = value;
			else if (strcmp(property, ".red") == 0)
				gTiles[arg_val].red = value;
			else if (strcmp(property, ".green") == 0)
				gTiles[arg_val].green = value;
			else if (strcmp(property, ".blue") == 0)
				gTiles[arg_val].blue = value;
			else if (strcmp(property, ".recovery_timer") == 0)
				gTiles[arg_val].recovery_timer = (int)value;
			else if (strcmp(property, ".state") == 0)
				gTiles[arg_val].state = (SDL_bool)value;
			else if (strcmp(property, ".is_dead") == 0)
				gTiles[arg_val].isDead = (SDL_bool)value;
			else
			{
				// No value was set
				*errorFlag = SDL_TRUE;
			}
			
			return value;
		}
		
		// set commands with no arguements
		
		// redRover
		if (strcmp(input, "scc~: redRover.x") == 0)
		{
			gRedRover.x = value;
		}
		else if (strcmp(input, "scc~: redRover.y") == 0)
		{
			gRedRover.y = value;
		}
		else if (strcmp(input, "scc~: redRover.z") == 0)
		{
			gRedRover.z = value;
		}
		else if (strcmp(input, "scc~: redRover.xrot") == 0)
		{
			gRedRover.xRot = value;
		}
		else if (strcmp(input, "scc~: redRover.yrot") == 0)
		{
			gRedRover.yRot = value;
		}
		else if (strcmp(input, "scc~: redRover.zrot") == 0)
		{
			gRedRover.zRot = value;
		}
		else if (strcmp(input, "scc~: redRover.direction") == 0)
		{
			gRedRover.direction = (int)value;
		}
		else if (strcmp(input, "scc~: redRover.state") == 0)
		{
			gRedRover.state = (int)value;
		}
		else if (strcmp(input, "scc~: redRover.recovery_timer") == 0)
		{
			gRedRover.recovery_timer = (int)value;
		}
		else if (strcmp(input, "scc~: redRover.animation_timer") == 0)
		{
			gRedRover.animation_timer = (int)value;
		}
		else if (strcmp(input, "scc~: redRover.weapon.x") == 0)
		{
			gRedRover.weap->x = value;
		}
		else if (strcmp(input, "scc~: redRover.weapon.y") == 0)
		{
			gRedRover.weap->y = value;
		}
		else if (strcmp(input, "scc~: redRover.weapon.z") == 0)
		{
			gRedRover.weap->z = value;
		}
		else if (strcmp(input, "scc~: redRover.weapon.direction") == 0)
		{
			gRedRover.weap->direction = (int)value;
		}
		else if (strcmp(input, "scc~: redRover.weapon.drawing_state") == 0)
		{
			gRedRover.weap->drawingState = (SDL_bool)value;
		}
		else if (strcmp(input, "scc~: redRover.weapon.animation_state") == 0)
		{
			gRedRover.weap->animationState = (SDL_bool)value;
		}
		else if (strcmp(input, "scc~: redRover.weapon.red") == 0)
		{
			gRedRover.weap->red = value;
		}
		else if (strcmp(input, "scc~: redRover.weapon.green") == 0)
		{
			gRedRover.weap->green = value;
		}
		else if (strcmp(input, "scc~: redRover.weapon.blue") == 0)
		{
			gRedRover.weap->blue = value;
		}
		
		// greenTree
		else if (strcmp(input, "scc~: greenTree.x") == 0)
		{
			gGreenTree.x = value;
		}
		else if (strcmp(input, "scc~: greenTree.y") == 0)
		{
			gGreenTree.y = value;
		}
		else if (strcmp(input, "scc~: greenTree.z") == 0)
		{
			gGreenTree.z = value;
		}
		else if (strcmp(input, "scc~: greenTree.xrot") == 0)
		{
			gGreenTree.xRot = value;
		}
		else if (strcmp(input, "scc~: greenTree.yrot") == 0)
		{
			gGreenTree.yRot = value;
		}
		else if (strcmp(input, "scc~: greenTree.zrot") == 0)
		{
			gGreenTree.zRot = value;
		}
		else if (strcmp(input, "scc~: greenTree.direction") == 0)
		{
			gGreenTree.direction = (int)value;
		}
		else if (strcmp(input, "scc~: greenTree.state") == 0)
		{
			gGreenTree.state = (int)value;
		}
		else if (strcmp(input, "scc~: greenTree.recovery_timer") == 0)
		{
			gGreenTree.recovery_timer = (int)value;
		}
		else if (strcmp(input, "scc~: greenTree.animation_timer") == 0)
		{
			gGreenTree.animation_timer = (int)value;
		}
		else if (strcmp(input, "scc~: greenTree.weapon.x") == 0)
		{
			gGreenTree.weap->x = value;
		}
		else if (strcmp(input, "scc~: greenTree.weapon.y") == 0)
		{
			gGreenTree.weap->y = value;
		}
		else if (strcmp(input, "scc~: greenTree.weapon.z") == 0)
		{
			gGreenTree.weap->z = value;
		}
		else if (strcmp(input, "scc~: greenTree.weapon.direction") == 0)
		{
			gGreenTree.weap->direction = (int)value;
		}
		else if (strcmp(input, "scc~: greenTree.weapon.drawing_state") == 0)
		{
			gGreenTree.weap->drawingState = (SDL_bool)value;
		}
		else if (strcmp(input, "scc~: greenTree.weapon.animation_state") == 0)
		{
			gGreenTree.weap->animationState = (SDL_bool)value;
		}
		else if (strcmp(input, "scc~: greenTree.weapon.red") == 0)
		{
			gGreenTree.weap->red = value;
		}
		else if (strcmp(input, "scc~: greenTree.weapon.green") == 0)
		{
			gGreenTree.weap->green = value;
		}
		else if (strcmp(input, "scc~: greenTree.weapon.blue") == 0)
		{
			gGreenTree.weap->blue = value;
		}
		
		// pinkBubbleGum
		else if (strcmp(input, "scc~: pinkBubbleGum.x") == 0)
		{
			gPinkBubbleGum.x = value;
		}
		else if (strcmp(input, "scc~: pinkBubbleGum.y") == 0)
		{
			gPinkBubbleGum.y = value;
		}
		else if (strcmp(input, "scc~: pinkBubbleGum.z") == 0)
		{
			gPinkBubbleGum.z = value;
		}
		else if (strcmp(input, "scc~: pinkBubbleGum.xrot") == 0)
		{
			gPinkBubbleGum.xRot = value;
		}
		else if (strcmp(input, "scc~: pinkBubbleGum.yrot") == 0)
		{
			gPinkBubbleGum.yRot = value;
		}
		else if (strcmp(input, "scc~: pinkBubbleGum.zrot") == 0)
		{
			gPinkBubbleGum.zRot = value;
		}
		else if (strcmp(input, "scc~: pinkBubbleGum.direction") == 0)
		{
			gPinkBubbleGum.direction = (int)value;
		}
		else if (strcmp(input, "scc~: pinkBubbleGum.state") == 0)
		{
			gPinkBubbleGum.state = (int)value;
		}
		else if (strcmp(input, "scc~: pinkBubbleGum.recovery_timer") == 0)
		{
			gPinkBubbleGum.recovery_timer = (int)value;
		}
		else if (strcmp(input, "scc~: pinkBubbleGum.animation_timer") == 0)
		{
			gPinkBubbleGum.animation_timer = (int)value;
		}
		else if (strcmp(input, "scc~: pinkBubbleGum.weapon.x") == 0)
		{
			gPinkBubbleGum.weap->x = value;
		}
		else if (strcmp(input, "scc~: pinkBubbleGum.weapon.y") == 0)
		{
			gPinkBubbleGum.weap->y = value;
		}
		else if (strcmp(input, "scc~: pinkBubbleGum.weapon.z") == 0)
		{
			gPinkBubbleGum.weap->z = value;
		}
		else if (strcmp(input, "scc~: pinkBubbleGum.weapon.direction") == 0)
		{
			gPinkBubbleGum.weap->direction = (int)value;
		}
		else if (strcmp(input, "scc~: pinkBubbleGum.weapon.drawing_state") == 0)
		{
			gPinkBubbleGum.weap->drawingState = (SDL_bool)value;
		}
		else if (strcmp(input, "scc~: pinkBubbleGum.weapon.animation_state") == 0)
		{
			gPinkBubbleGum.weap->animationState = (SDL_bool)value;
		}
		else if (strcmp(input, "scc~: pinkBubbleGum.weapon.red") == 0)
		{
			gPinkBubbleGum.weap->red = value;
		}
		else if (strcmp(input, "scc~: pinkBubbleGum.weapon.green") == 0)
		{
			gPinkBubbleGum.weap->green = value;
		}
		else if (strcmp(input, "scc~: pinkBubbleGum.weapon.blue") == 0)
		{
			gPinkBubbleGum.weap->blue = value;
		}
		
		// blueLightning
		else if (strcmp(input, "scc~: blueLightning.x") == 0)
		{
			gBlueLightning.x = value;
		}
		else if (strcmp(input, "scc~: blueLightning.y") == 0)
		{
			gBlueLightning.y = value;
		}
		else if (strcmp(input, "scc~: blueLightning.z") == 0)
		{
			gBlueLightning.z = value;
		}
		else if (strcmp(input, "scc~: blueLightning.xrot") == 0)
		{
			gBlueLightning.xRot = value;
		}
		else if (strcmp(input, "scc~: blueLightning.yrot") == 0)
		{
			gBlueLightning.yRot = value;
		}
		else if (strcmp(input, "scc~: blueLightning.zrot") == 0)
		{
			gBlueLightning.zRot = value;
		}
		else if (strcmp(input, "scc~: blueLightning.direction") == 0)
		{
			gBlueLightning.direction = (int)value;
		}
		else if (strcmp(input, "scc~: blueLightning.state") == 0)
		{
			gBlueLightning.state = (int)value;
		}
		else if (strcmp(input, "scc~: blueLightning.recovery_timer") == 0)
		{
			gBlueLightning.recovery_timer = (int)value;
		}
		else if (strcmp(input, "scc~: blueLightning.animation_timer") == 0)
		{
			gBlueLightning.animation_timer = (int)value;
		}
		else if (strcmp(input, "scc~: blueLightning.weapon.x") == 0)
		{
			gBlueLightning.weap->x = value;
		}
		else if (strcmp(input, "scc~: blueLightning.weapon.y") == 0)
		{
			gBlueLightning.weap->y = value;
		}
		else if (strcmp(input, "scc~: blueLightning.weapon.z") == 0)
		{
			gBlueLightning.weap->z = value;
		}
		else if (strcmp(input, "scc~: blueLightning.weapon.direction") == 0)
		{
			gBlueLightning.weap->direction = (int)value;
		}
		else if (strcmp(input, "scc~: blueLightning.weapon.drawing_state") == 0)
		{
			gBlueLightning.weap->drawingState = (SDL_bool)value;
		}
		else if (strcmp(input, "scc~: blueLightning.weapon.animation_state") == 0)
		{
			gBlueLightning.weap->animationState = (SDL_bool)value;
		}
		else if (strcmp(input, "scc~: blueLightning.weapon.red") == 0)
		{
			gBlueLightning.weap->red = value;
		}
		else if (strcmp(input, "scc~: blueLightning.weapon.green") == 0)
		{
			gBlueLightning.weap->green = value;
		}
		else if (strcmp(input, "scc~: blueLightning.weapon.blue") == 0)
		{
			gBlueLightning.weap->blue = value;
		}
		else if (strcmp(input, "scc~: ai_mode") == 0)
		{
			gAIMode = (int)value;
		}
		else if (strcmp(input, "scc~: game_reset") == 0)
		{
			if (!gNetworkConnection || gNetworkConnection->type != NETWORK_CLIENT_TYPE)
			{
				if (valueExists)
				{
					gGameShouldReset = value;
				}
				else
				{
					gGameShouldReset = SDL_TRUE;
					value = gGameShouldReset;
				}

				
				if (gNetworkConnection && gNetworkConnection->type == NETWORK_SERVER_TYPE)
				{
					sendToClients(0, "ng");
				}
			}
		}
	}
	
	if (strcmp(input, "scc~: fps") == 0)
	{
		if (valueExists)
		{
			gDrawFPS = (SDL_bool)value;
		}
		else
		{
			gDrawFPS = !gDrawFPS;
			value = gDrawFPS;
		}
	}
	
	return value;
}

void clearConsole(void)
{
	if (gConsoleStringIndex > MIN_CONSOLE_STRING_LENGTH)
	{
		if (gConsoleString[gConsoleStringIndex - 1] == ' ')
		{
			performConsoleBackspace();
		}
		
		while (gConsoleStringIndex > MIN_CONSOLE_STRING_LENGTH && gConsoleString[gConsoleStringIndex - 1] != ' ' && performConsoleBackspace())
		{
		}
		
		// if we are trying to get something, we want to get rid of the extra space
		if (gConsoleStringIndex > MIN_CONSOLE_STRING_LENGTH + 1 && memcmp("get", gConsoleString + MIN_CONSOLE_STRING_LENGTH, strlen("get")) == 0 && gConsoleString[gConsoleStringIndex - 1] == ' ')
		{
			performConsoleBackspace();
		}
	}
}