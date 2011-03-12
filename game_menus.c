/*
 * Coder: Mayur Pawashe (Zorg)
 * http://zorg.tejat.net
 */

#include "game_menus.h"
#include "input.h"
#include "font.h"
#include "network.h"
#include "audio.h"
#include "utilities.h" // for SDL_Terminate()

// Four characters that each have five configured menu actions (right, up, left, down, fire/slicing knife)
Menu gCharacterConfigureKeys[4][6];
Menu gJoyStickConfig[4][6];
Menu *gVideoOptionsMenu;

Menu *gConfigureLivesMenu;
Menu *gScreenResolutionVideoOptionMenu;
Menu *gRefreshRateVideoOptionMenu;

Menu *gAIModeOptionsMenu;
Menu *gAINetModeOptionsMenu;

Menu *gRedRoverPlayerOptionsMenu;
Menu *gGreenTreePlayerOptionsMenu;
Menu *gBlueLightningPlayerOptionsMenu;

SDL_bool gDrawArrowsForCharacterLivesFlag =		SDL_FALSE;
SDL_bool gDrawArrowsForScreenResolutionsFlag =	SDL_FALSE;
SDL_bool gDrawArrowsForRefreshRatesFlag =		SDL_FALSE;
SDL_bool gDrawArrowsForAIModeFlag =				SDL_FALSE;
SDL_bool gDrawArrowsForNumberOfNetHumansFlag =	SDL_FALSE;
SDL_bool gDrawArrowsForNetPlayerLivesFlag =		SDL_FALSE;

SDL_bool gNetworkAddressFieldIsActive =			SDL_FALSE;
SDL_bool gNetworkUserNameFieldIsActive =		SDL_FALSE;

// Variable that holds a keyCode for configuring keys (see convertKeyCodeToString() and menu configuration implementations)
static char gKeyCode[15];

char gServerAddressString[MAX_SERVER_ADDRESS_SIZE] = "127.0.0.1";
int gServerAddressStringIndex = 9;

char gUserNameString[MAX_USER_NAME_SIZE] = "Borg";
int gUserNameStringIndex = 4;

static char *convertKeyCodeToString(unsigned theKeyCode);
static unsigned getKey(void);
static unsigned getJoyStickTrigger(Sint16 *value, Uint8 *axis, int *joy_id);

static void drawUpAndDownArrowTriangles(void)
{
	glEnableClientState(GL_VERTEX_ARRAY);
	
	GLfloat vertices[] =
	{
		0.0, 0.6, 0.0,
		-0.2, 0.2, 0.0,
		0.2, 0.2, 0.0,
		
		0.0, -0.6, 0.0,
		-0.2, -0.2, 0.0,
		0.2, -0.2, 0.0,
	};
	
	glVertexPointer(3, GL_FLOAT, 0, vertices);
	glDrawArrays(GL_TRIANGLES, 0, 6);
	
	glDisableClientState(GL_VERTEX_ARRAY);
}

/* A bunch of menu drawing and action functions! */

void drawPlayMenu(void)
{
	glTranslatef(-1.0, 15.0, -280.0);
	
	drawString(12.0, 5.0, "Play");
	
	glLoadIdentity();
}

void drawNetworkPlayMenu(void)
{
	glTranslatef(-1.0, 0.0, -280.0);
	
	drawString(20.0, 5.0, "Network Play");
	
	glLoadIdentity();
}

void networkPlayMenuAction(void)
{
	changeMenu(RIGHT);
}

void drawNetworkUserNameFieldMenu(void)
{
	glTranslatef(-5.0, -15.0, -280.0);
	
	drawString(10.0, 5.0, "Name:");
	
	if (gNetworkUserNameFieldIsActive)
	{
		glColor4f(0.0, 0.0, 0.8, 1.0);
	}
	
	glTranslatef(strlen(gUserNameString) + 15.0, 0.0, 0.0);
	if (strlen(gUserNameString) > 0)
	{
		drawString(strlen(gUserNameString) + 2.0, 5.0, gUserNameString);
	}
	
	glLoadIdentity();
}

void networkUserNameFieldMenuAction(void)
{
	gNetworkUserNameFieldIsActive = !gNetworkUserNameFieldIsActive;
}

void writeNetworkUserNameText(Uint8 text)
{
	if (gUserNameStringIndex < MAX_USER_NAME_SIZE - 1)
	{
		gUserNameString[gUserNameStringIndex] = text;
		gUserNameString[gUserNameStringIndex + 1] = '\0';
		
		gUserNameStringIndex++;
	}
}

void performNetworkUserNameBackspace(void)
{	
	if (gUserNameStringIndex > 0)
	{
		gUserNameStringIndex--;
		gUserNameString[gUserNameStringIndex] = '\0';
	}
}

void drawNetworkServerMenu(void)
{
	glTranslatef(-1.0, 15.0, -280.0);
	
	drawString(15.0, 5.0, "Server");
	
	glLoadIdentity();
}

void networkServerMenuAction(void)
{
	changeMenu(RIGHT);
}

void drawNetworkServerPlayMenu(void)
{
	glTranslatef(-20.0, 0.0, -280.0);
	
	drawString(20.0, 5.0, "Start Game");
	
	glLoadIdentity();
}

void networkServerPlayMenuAction(void)
{
	gPinkBubbleGum.backup_state = gPinkBubbleGum.state;
	gRedRover.backup_state = gRedRover.state;
	gGreenTree.backup_state = gGreenTree.state;
	gBlueLightning.backup_state = gBlueLightning.state;
	
	gPinkBubbleGum.state = CHARACTER_HUMAN_STATE;
	gRedRover.state = CHARACTER_HUMAN_STATE;
	gGreenTree.state = gNumberOfNetHumans > 1 ? CHARACTER_HUMAN_STATE : CHARACTER_AI_STATE;
	gBlueLightning.state = gNumberOfNetHumans > 2 ? CHARACTER_HUMAN_STATE : CHARACTER_AI_STATE;
	
	gPinkBubbleGum.netState = NETWORK_PLAYING_STATE;
	gRedRover.netState = gRedRover.state == CHARACTER_HUMAN_STATE ? NETWORK_PENDING_STATE : NETWORK_PLAYING_STATE;
	gGreenTree.netState = gGreenTree.state == CHARACTER_HUMAN_STATE ? NETWORK_PENDING_STATE : NETWORK_PLAYING_STATE;
	gBlueLightning.netState = gBlueLightning.state == CHARACTER_HUMAN_STATE ? NETWORK_PENDING_STATE : NETWORK_PLAYING_STATE;
	
	gNetworkConnection = malloc(sizeof(NetworkConnection));
	gNetworkConnection->type = NETWORK_SERVER_TYPE;
	gNetworkConnection->input = &gPinkBubbleGumInput;
	gPinkBubbleGum.netName = gUserNameString;
	gNetworkConnection->shouldRun = SDL_TRUE;
	
	gNetworkConnection->numberOfPlayersToWaitFor = 0; 
 	gNetworkConnection->numberOfPlayersToWaitFor += (gRedRover.netState == NETWORK_PENDING_STATE);
 	gNetworkConnection->numberOfPlayersToWaitFor += (gGreenTree.netState == NETWORK_PENDING_STATE);
 	gNetworkConnection->numberOfPlayersToWaitFor += (gBlueLightning.netState == NETWORK_PENDING_STATE);
	
	networkInitialization();
	
	// open socket
	gNetworkConnection->socket = socket(AF_INET, SOCK_DGRAM, 0);
	
	if (gNetworkConnection->socket == -1)
	{
		perror("socket");
		
		free(gNetworkConnection);
		gNetworkConnection = NULL;
		
		restoreAllBackupStates();
		
		networkCleanup();
		
		return;
	}
	
	struct sockaddr_in address;
	
	// set address
	address.sin_family = AF_INET;
    address.sin_port = htons(NETWORK_PORT);
    address.sin_addr.s_addr = INADDR_ANY;
    memset(address.sin_zero, '\0', sizeof(address.sin_zero));
	
	// bind
	if (bind(gNetworkConnection->socket, (struct sockaddr *)&address, sizeof(address)) == -1)
	{
        perror("bind");
		
		closeSocket(gNetworkConnection->socket);
		free(gNetworkConnection);
		gNetworkConnection = NULL;
		
		restoreAllBackupStates();
		
		networkCleanup();
		
		return;
    }
	
	SDL_CreateThread(serverNetworkThread, NULL);
	
	initGame();
	
	gRedRoverInput.character = gNetworkConnection->input->character;
	gBlueLightningInput.character = gNetworkConnection->input->character;
	gGreenTreeInput.character = gNetworkConnection->input->character;
	
	// make sure we are at main menu
	changeMenu(LEFT);
	changeMenu(LEFT);
}

void drawNetworkServerNumberOfPlayersMenu(void)
{
	glTranslatef(-20.0, -15.0, -280.0);
	
	drawStringf(20.0, 5.0, "Net-Players: %i", gNumberOfNetHumans);
	
	glLoadIdentity();
	
	if (gDrawArrowsForNumberOfNetHumansFlag)
	{
		glTranslatef(0.35, -1.3, -25.0);
		glColor3f(0.0, 0.0, 0.4);
		
		drawUpAndDownArrowTriangles();
		
		glLoadIdentity();
	}
}

void networkServerNumberOfPlayersMenuAction(void)
{
	gDrawArrowsForNumberOfNetHumansFlag = !gDrawArrowsForNumberOfNetHumansFlag;
}

void drawNetworkServerAIModeMenu(void)
{
	glTranslatef(-20.0, -30.0, -280.0);
	
	if (gAINetMode == AI_EASY_MODE)
		drawString(20.0, 5.0, "Bot Mode: Easy");
	
	else if (gAINetMode == AI_MEDIUM_MODE)
		drawString(20.0, 5.0, "Bot Mode: Medium");
	
	else if (gAINetMode == AI_HARD_MODE)
		drawString(20.0, 5.0, "Bot Mode: Hard");
	
	glLoadIdentity();
	
	if (gDrawArrowsForAIModeFlag)
	{
		glTranslatef(0.35, -2.7, -25.0);
		glColor3f(0.0, 0.0, 0.4);
		
		drawUpAndDownArrowTriangles();
		
		glLoadIdentity();
	}
}

void networkServerAIModeMenuAction(void)
{
}

void drawNetworkServerPlayerLivesMenu(void)
{
	glTranslatef(-20.0, -45.0, -280.0);
	
	drawStringf(20.0, 5.0, "Player Lives: %i", gCharacterNetLives);
	
	glLoadIdentity();
	
	if (gDrawArrowsForNetPlayerLivesFlag)
	{
		glTranslatef(0.35, -4.0, -25.0);
		glColor3f(0.0, 0.0, 0.4);
		
		drawUpAndDownArrowTriangles();
		
		glLoadIdentity();
	}
}

void networkServerPlayerLivesMenuAction(void)
{
	gDrawArrowsForNetPlayerLivesFlag = !gDrawArrowsForNetPlayerLivesFlag;
}

void drawNetworkClientMenu(void)
{
	glTranslatef(-1.0, 0.0, -280.0);
	
	drawString(15.0, 5.0, "Client");
	
	glLoadIdentity();
}

void networkClientMenuAction(void)
{
	changeMenu(RIGHT);
}

void drawNetworkAddressFieldMenu(void)
{
	glTranslatef(-40.0, 0.0, -280.0);
	
	drawString(20.0, 5.0, "Host Address:");
	
	if (gNetworkAddressFieldIsActive)
		glColor4f(0.0, 0.0, 0.8, 1.0);
	
	glTranslatef(strlen(gServerAddressString) + 28.0, 0.0, 0.0);
	if (strlen(gServerAddressString) > 0)
	{
		drawString(strlen(gServerAddressString) + 5.0, 5.0, gServerAddressString);
	}
	
	glLoadIdentity();
}

void networkAddressFieldMenuAction(void)
{
	gNetworkAddressFieldIsActive = !gNetworkAddressFieldIsActive;
}

void writeNetworkAddressText(Uint8 text)
{
	if (gServerAddressStringIndex < MAX_SERVER_ADDRESS_SIZE - 1)
	{
		gServerAddressString[gServerAddressStringIndex] = text;
		gServerAddressString[gServerAddressStringIndex + 1] = '\0';
		
		gServerAddressStringIndex++;
	}
}

void performNetworkAddressBackspace(void)
{	
	if (gServerAddressStringIndex > 0)
	{
		gServerAddressStringIndex--;
		gServerAddressString[gServerAddressStringIndex] = '\0';
	}
}

void drawConnectToNetworkGameMenu(void)
{
	glTranslatef(-45.0, -15.0, -280.0);
	
	drawString(15.0, 5.0, "Connect");
	
	glLoadIdentity();
}

void connectToNetworkGameMenuAction(void)
{
	if (gNetworkConnection && gNetworkConnection->shouldRun)
	{
		// the thread already exists, wait for it to die
		gNetworkConnection->shouldRun = SDL_FALSE;
		SDL_WaitThread(gNetworkConnection->thread, NULL);
	}
	else if (gNetworkConnection && !(gNetworkConnection->shouldRun))
	{
		SDL_WaitThread(gNetworkConnection->thread, NULL);
	}
	
	gNetworkConnection = malloc(sizeof(NetworkConnection));
	gNetworkConnection->input = NULL;
	gNetworkConnection->isConnected = SDL_FALSE;
	gNetworkConnection->type = NETWORK_CLIENT_TYPE;
	gNetworkConnection->numberOfPlayersToWaitFor = 0;
	gNetworkConnection->shouldRun = SDL_TRUE;
	
	networkInitialization();
	
	// open socket
	gNetworkConnection->socket = socket(AF_INET, SOCK_DGRAM, 0);
	
	if (gNetworkConnection->socket == -1)
	{
		perror("socket");
		
		free(gNetworkConnection);
		gNetworkConnection = NULL;
		
		networkCleanup();
		
		return;
	}
	
	struct hostent *host_entry = gethostbyname(gServerAddressString);
	
	if (host_entry == NULL)
	{
#ifndef WINDOWS
		herror("gethostbyname");
#else
		zgPrint("host_entry windows error\n");
#endif
		
		closeSocket(gNetworkConnection->socket);
		free(gNetworkConnection);
		gNetworkConnection = NULL;
		
		networkCleanup();
		
		return;
	}
	
	// set address
	gNetworkConnection->hostAddress.sin_family = AF_INET;
	gNetworkConnection->hostAddress.sin_port = htons(NETWORK_PORT);
	gNetworkConnection->hostAddress.sin_addr = *((struct in_addr *)host_entry->h_addr);
	memset(gNetworkConnection->hostAddress.sin_zero, '\0', sizeof(gNetworkConnection->hostAddress.sin_zero));
	
	gNetworkConnection->thread = SDL_CreateThread(clientNetworkThread, NULL);
}

void drawGameOptionsMenu(void)
{
	glTranslatef(-1.0, -15.0, -280.0);
	
	drawString(20.0, 5.0, "Game Options");
	
	glLoadIdentity();
}

void gameOptionsMenuAction(void)
{
	changeMenu(RIGHT);
}

void drawPlayerOptionsMenu(void)
{
	glTranslatef(-1.0, 15.0, -280.0);
	
	drawString(20.0, 5.0, "Configure Players");
	
	glLoadIdentity();
}

void playerOptionsMenuAction(void)
{
	changeMenu(RIGHT);
}

void drawConfigureKeysMenu(void)
{
	glTranslatef(-1.0, 0.0, -280.0);
	
	drawString(20.0, 5.0, "Configure Keys");
	
	glLoadIdentity();
}

void configureKeysMenuAction(void)
{
	changeMenu(RIGHT);
}

void drawJoySticksConfigureMenu(void)
{
	glTranslatef(-1.0, -15.0, -280.0);
	
	drawString(20.0, 5.0, "Configure Joy Sticks");
	
	glLoadIdentity();
}

void joySticksConfigureMenuAction(void)
{
	changeMenu(RIGHT);
}

void drawPinkBubbleGumPlayerOptionsMenu(void)
{
	glTranslatef(-1.0, 15.0, -280.0);
	
	if (gPinkBubbleGum.state == CHARACTER_HUMAN_STATE)
		drawString(20.0, 5.0, "Pink Bubblegum: Human");
	else if (gPinkBubbleGum.state == CHARACTER_AI_STATE)
		drawString(20.0, 5.0, "Pink Bubblegum: Bot");
	
	glLoadIdentity();
}

void pinkBubbleGumPlayerOptionsMenuAction(void)
{
	if (gPinkBubbleGum.state == CHARACTER_HUMAN_STATE)
	{
		gPinkBubbleGum.state = CHARACTER_AI_STATE;
		gRedRover.state = CHARACTER_AI_STATE;
		gGreenTree.state = CHARACTER_AI_STATE;
		gBlueLightning.state = CHARACTER_AI_STATE;
	}
	else
	{
		gPinkBubbleGum.state = CHARACTER_HUMAN_STATE;
	}
}

void drawRedRoverPlayerOptionsMenu(void)
{
	glTranslatef(-1.0, 0.0, -280.0);
	
	if (gPinkBubbleGum.state == CHARACTER_AI_STATE)
	{
		glColor4f(0.0, 0.0, 0.0, 0.5);
	}
	
	if (gRedRover.state == CHARACTER_HUMAN_STATE)
		drawString(20.0, 5.0, "Red Rover: Human");
	else
		drawString(20.0, 5.0, "Red Rover: Bot");
	
	glLoadIdentity();
}

void redRoverPlayerOptionsMenuAction(void)
{
	if (gRedRover.state == CHARACTER_HUMAN_STATE)
	{
		gRedRover.state = CHARACTER_AI_STATE;
		gGreenTree.state = CHARACTER_AI_STATE;
		gBlueLightning.state = CHARACTER_AI_STATE;
	}
	else
	{
		gRedRover.state = CHARACTER_HUMAN_STATE;
	}
}

void drawGreenTreePlayerOptionsMenu(void)
{
	glTranslatef(-1.0, -15.0, -280.0);
	
	if (gRedRover.state == CHARACTER_AI_STATE)
	{
		glColor4f(0.0, 0.0, 0.0, 0.5);
	}
	
	if (gGreenTree.state == CHARACTER_HUMAN_STATE)
	{
		drawString(20.0, 5.0, "Green Tree: Human");
	}
	else if (gGreenTree.state == CHARACTER_AI_STATE)
		drawString(20.0, 5.0, "Green Tree: Bot");
	
	glLoadIdentity();
}

void greenTreePlayerOptionsMenuAction(void)
{
	if (gGreenTree.state == CHARACTER_HUMAN_STATE)
	{
		gGreenTree.state = CHARACTER_AI_STATE;
		gBlueLightning.state = CHARACTER_AI_STATE;
	}
	else
	{
		gGreenTree.state = CHARACTER_HUMAN_STATE;
	}
}

void drawBlueLightningPlayerOptionsMenu(void)
{
	glTranslatef(-1.0, -30.0, -280.0);
	
	if (gGreenTree.state == CHARACTER_AI_STATE)
	{
		glColor4f(0.0, 0.0, 0.0, 0.5);
	}
	
	if (gBlueLightning.state == CHARACTER_HUMAN_STATE)
		drawStringf(20.0, 5.0, "Blue Lightning: Human");
	else if (gBlueLightning.state == CHARACTER_AI_STATE)
		drawString(20.0, 5.0, "Blue Lightning: Bot");
	
	glLoadIdentity();
}

void blueLightningPlayerOptionsMenuAction(void)
{
	if (gBlueLightning.state == CHARACTER_HUMAN_STATE)
	{
		gBlueLightning.state = CHARACTER_AI_STATE;
	}
	else
	{
		gBlueLightning.state = CHARACTER_HUMAN_STATE;
	}
}

void drawAIModeOptionsMenu(void)
{
	glTranslatef(-1.0, -45.0, -280.0);
	
	if (gAIMode == AI_EASY_MODE)
		drawString(20.0, 5.0, "Bot Mode: Easy");
	
	else if (gAIMode == AI_MEDIUM_MODE)
		drawString(20.0, 5.0, "Bot Mode: Medium");
	
	else if (gAIMode == AI_HARD_MODE)
		drawString(20.0, 5.0, "Bot Mode: Hard");
	
	glLoadIdentity();
	
	if (gDrawArrowsForAIModeFlag)
	{
		glTranslatef(2.0, -4.1, -25.0);
		glColor3f(0.0, 0.0, 0.4);
		
		drawUpAndDownArrowTriangles();
		
		glLoadIdentity();
	}
}

void drawConfigureLivesMenu(void)
{
	glTranslatef(-1.0, -60.0, -280.0);
	
	drawStringf(20.0, 5.0, "Player Lives: %i", gCharacterLives);
	
	glLoadIdentity();
	
	if (gDrawArrowsForCharacterLivesFlag)
	{
		glTranslatef(2.0, -5.4, -25.0);
		glColor3f(0.0, 0.0, 0.4);
		
		drawUpAndDownArrowTriangles();
		
		glLoadIdentity();
	}
}

void drawPinkBubbleGumConfigKey(void)
{	
	glTranslatef(-1.0, 15.0, -280.0);
	
	drawString(15.0, 5.0, "Pink Bubblegum");
	
	glLoadIdentity();
}

void pinkBubbleGumKeyMenuAction(void)
{
	changeMenu(RIGHT);
}

void drawRedRoverConfigKey(void)
{	
	glTranslatef(-1.0, 0.0, -280.0);
	
	drawString(15.0, 5.0, "Red Rover");
	
	glLoadIdentity();
}

void redRoverKeyMenuAction(void)
{
	changeMenu(RIGHT);
}

void drawGreenTreeConfigKey(void)
{	
	glTranslatef(-1.0, -15.0, -280.0);
	
	drawString(15.0, 5.0, "Green Tree");
	
	glLoadIdentity();
}

void greenTreeKeyMenuAction(void)
{
	changeMenu(RIGHT);
}

void drawBlueLightningConfigKey(void)
{	
	glTranslatef(-1.0, -30.0, -280.0);
	
	drawString(15.0, 5.0, "Blue Lightning");
	
	glLoadIdentity();
}

void blueLightningKeyMenuAction(void)
{
	changeMenu(RIGHT);
}

void configureKey(unsigned *id)
{
	unsigned key = getKey();
	
	if (key != SDLK_UNKNOWN)
		*id = key;
}

// start configuration menus
void drawPinkBubbleGumConfigRightKey(void)
{
	glTranslatef(-1.0, 15.0, -280.0);
	
	drawStringf(15.0, 5.0, "Right: %s", convertKeyCodeToString(gPinkBubbleGumInput.r_id));
	
	glLoadIdentity();
}

void pinkBubbleGumRightKeyMenuAction(void)
{	
	configureKey(&gPinkBubbleGumInput.r_id);
}

void drawPinkBubbleGumConfigLeftKey(void)
{
	glTranslatef(-1.0, 0.0, -280.0);
	
	drawStringf(15.0, 5.0, "Left: %s", convertKeyCodeToString(gPinkBubbleGumInput.l_id));
	
	glLoadIdentity();
}

void pinkBubbleGumLeftKeyMenuAction(void)
{
	configureKey(&gPinkBubbleGumInput.l_id);
}

void drawPinkBubbleGumConfigUpKey(void)
{
	glTranslatef(-1.0, -15.0, -280.0);
	
	drawStringf(15.0, 5.0, "Up: %s", convertKeyCodeToString(gPinkBubbleGumInput.u_id));
	
	glLoadIdentity();
}

void pinkBubbleGumUpKeyMenuAction(void)
{
	configureKey(&gPinkBubbleGumInput.u_id);
}

void drawPinkBubbleGumConfigDownKey(void)
{
	glTranslatef(-1.0, -30.0, -280.0);
	
	drawStringf(15.0, 5.0, "Down: %s", convertKeyCodeToString(gPinkBubbleGumInput.d_id));
	
	glLoadIdentity();
}

void pinkBubbleGumDownKeyMenuAction(void)
{
	configureKey(&gPinkBubbleGumInput.d_id);
}

void drawPinkBubbleGumConfigFireKey(void)
{
	glTranslatef(-1.0, -45.0, -280.0);
	
	drawStringf(15.0, 5.0, "Fire: %s", convertKeyCodeToString(gPinkBubbleGumInput.weap_id));
	
	glLoadIdentity();
}

void pinkBubbleGumFireKeyMenuAction(void)
{
	configureKey(&gPinkBubbleGumInput.weap_id);
}

void drawRedRoverConfigRightKey(void)
{
	glTranslatef(-1.0, 15.0, -280.0);
	
	drawStringf(15.0, 5.0, "Right: %s", convertKeyCodeToString(gRedRoverInput.r_id));
	
	glLoadIdentity();
}

void redRoverRightKeyMenuAction(void)
{
	configureKey(&gRedRoverInput.r_id);
}

void drawRedRoverConfigLeftKey(void)
{
	glTranslatef(-1.0, 0.0, -280.0);
	
	drawStringf(15.0, 5.0, "Left: %s", convertKeyCodeToString(gRedRoverInput.l_id));
	
	glLoadIdentity();
}

void redRoverLeftKeyMenuAction(void)
{
	configureKey(&gRedRoverInput.l_id);
}

void drawRedRoverConfigUpKey(void)
{
	glTranslatef(-1.0, -15.0, -280.0);
	
	drawStringf(15.0, 5.0, "Up: %s", convertKeyCodeToString(gRedRoverInput.u_id));
	
	glLoadIdentity();
}

void redRoverUpKeyMenuAction(void)
{
	configureKey(&gRedRoverInput.u_id);
}

void drawRedRoverConfigDownKey(void)
{
	glTranslatef(-1.0, -30.0, -280.0);
	
	drawStringf(15.0, 5.0, "Down: %s", convertKeyCodeToString(gRedRoverInput.d_id));
	
	glLoadIdentity();
}

void redRoverDownKeyMenuAction(void)
{
	configureKey(&gRedRoverInput.d_id);
}

void drawRedRoverConfigFireKey(void)
{
	glTranslatef(-1.0, -45.0, -280.0);
	
	drawStringf(15.0, 5.0, "Fire: %s", convertKeyCodeToString(gRedRoverInput.weap_id));
	
	glLoadIdentity();
}

void redRoverFireKeyMenuAction(void)
{
	configureKey(&gRedRoverInput.weap_id);
}

void drawGreenTreeConfigRightKey(void)
{
	glTranslatef(-1.0, 15.0, -280.0);
	
	drawStringf(15.0, 5.0, "Right: %s", convertKeyCodeToString(gGreenTreeInput.r_id));
	
	glLoadIdentity();
}

void greenTreeRightKeyMenuAction(void)
{
	configureKey(&gGreenTreeInput.r_id);
}

void drawGreenTreeConfigLeftKey(void)
{
	glTranslatef(-1.0, 0.0, -280.0);
	
	drawStringf(15.0, 5.0, "Left: %s", convertKeyCodeToString(gGreenTreeInput.l_id));
	
	glLoadIdentity();
}

void greenTreeLeftKeyMenuAction(void)
{
	configureKey(&gGreenTreeInput.l_id);
}

void drawGreenTreeConfigUpKey(void)
{
	glTranslatef(-1.0, -15.0, -280.0);
	
	drawStringf(15.0, 5.0, "Up: %s", convertKeyCodeToString(gGreenTreeInput.u_id));
	
	glLoadIdentity();
}

void greenTreeUpKeyMenuAction(void)
{
	configureKey(&gGreenTreeInput.u_id);
}

void drawGreenTreeConfigDownKey(void)
{
	glTranslatef(-1.0, -30.0, -280.0);
	
	drawStringf(15.0, 5.0, "Down: %s", convertKeyCodeToString(gGreenTreeInput.d_id));
	
	glLoadIdentity();
}

void greenTreeDownKeyMenuAction(void)
{
	configureKey(&gGreenTreeInput.d_id);
}

void drawGreenTreeConfigFireKey(void)
{
	glTranslatef(-1.0, -45.0, -280.0);
	
	drawStringf(15.0, 5.0, "Fire: %s", convertKeyCodeToString(gGreenTreeInput.weap_id));
	
	glLoadIdentity();
}

void greenTreeFireKeyMenuAction(void)
{
	configureKey(&gGreenTreeInput.weap_id);
}

void drawBlueLightningConfigRightKey(void)
{
	glTranslatef(-1.0, 15.0, -280.0);
	
	drawStringf(15.0, 5.0, "Right: %s", convertKeyCodeToString(gBlueLightningInput.r_id));
	
	glLoadIdentity();
}

void blueLightningRightKeyMenuAction(void)
{
	configureKey(&gBlueLightningInput.r_id);
}

void drawBlueLightningConfigLeftKey(void)
{
	glTranslatef(-1.0, 0.0, -280.0);
	
	drawStringf(15.0, 5.0, "Left: %s", convertKeyCodeToString(gBlueLightningInput.l_id));
	
	glLoadIdentity();
}

void blueLightningLeftKeyMenuAction(void)
{
	configureKey(&gBlueLightningInput.l_id);
}

void drawBlueLightningConfigUpKey(void)
{
	glTranslatef(-1.0, -15.0, -280.0);
	
	drawStringf(15.0, 5.0, "Up: %s", convertKeyCodeToString(gBlueLightningInput.u_id));
	
	glLoadIdentity();
}

void blueLightningUpKeyMenuAction(void)
{
	configureKey(&gBlueLightningInput.u_id);
}

void drawBlueLightningConfigDownKey(void)
{
	glTranslatef(-1.0, -30.0, -280.0);
	
	drawStringf(15.0, 5.0, "Down: %s", convertKeyCodeToString(gBlueLightningInput.d_id));
	
	glLoadIdentity();
}

void blueLightningDownKeyMenuAction(void)
{
	configureKey(&gBlueLightningInput.d_id);
}

void drawBlueLightningConfigFireKey(void)
{
	glTranslatef(-1.0, -45.0, -280.0);
	
	drawStringf(15.0, 5.0, "Fire: %s", convertKeyCodeToString(gBlueLightningInput.weap_id));
	
	glLoadIdentity();
}

void blueLightningFireKeyMenuAction(void)
{
	configureKey(&gBlueLightningInput.weap_id);
}

void drawPinkBubbleGumConfigJoyStick(void)
{
	glTranslatef(-1.0, 15.0, -280.0);
	
	drawString(15.0, 5.0, "Pink Bubblegum");
	
	glLoadIdentity();
}

// config joy sticks.
void pinkBubbleGumConfigJoyStickAction(void)
{
	changeMenu(RIGHT);
}

void configureJoyStick(Input *input, int type)
{
	Sint16 value;
	Uint8 axis;
	int joy_id = 0;
	
	unsigned trigger = getJoyStickTrigger(&value, &axis, &joy_id);
	
	if (type == RIGHT)
	{
		input->joy_right_id = joy_id;
		
		if (value != (signed)JOY_NONE)
			input->rjs_id = value;
		
		input->rjs_axis_id = axis;
		
		if (axis != JOY_AXIS_NONE)
		{
			if (trigger == JOY_UP)
			{
				sprintf(input->joy_right, "Joy Up");
			} else if (trigger == JOY_RIGHT)
			{
				sprintf(input->joy_right, "Joy Right");
			} else if (trigger == JOY_DOWN)
			{
				sprintf(input->joy_right, "Joy Down");
			} else if (trigger == JOY_LEFT)
			{
				sprintf(input->joy_right, "Joy Left");
			}			
		}
		else
		{
			sprintf(input->joy_right, "Joy Button %i", input->rjs_id);
		}
	}
	
	else if (type == LEFT)
	{
		input->joy_left_id = joy_id;
		
		if (value != (signed)JOY_NONE)
			input->ljs_id = value;
		
		input->ljs_axis_id = axis;
		
		if (axis != JOY_AXIS_NONE)
		{
			if (trigger == JOY_UP)
			{
				sprintf(input->joy_left, "Joy Up");
			}
			else if (trigger == JOY_RIGHT)
			{
				sprintf(input->joy_left, "Joy Right");
			}
			else if (trigger == JOY_DOWN)
			{
				sprintf(input->joy_left, "Joy Down");
			}
			else if (trigger == JOY_LEFT)
			{
				sprintf(input->joy_left, "Joy Left");
			}			
		}
		else
		{
			sprintf(input->joy_left, "Joy Button %i", input->ljs_id);
		}
	}
	
	else if (type == UP)
	{
		input->joy_up_id = joy_id;
		
		if (value != (signed)JOY_NONE)
			input->ujs_id = value;
		
		input->ujs_axis_id = axis;
		
		if (axis != JOY_AXIS_NONE)
		{
			if (trigger == JOY_UP)
			{
				sprintf(input->joy_up, "Joy Up");
			}
			else if (trigger == JOY_RIGHT)
			{
				sprintf(input->joy_up, "Joy Right");
			}
			else if (trigger == JOY_DOWN)
			{
				sprintf(input->joy_up, "Joy Down");
			}
			else if (trigger == JOY_LEFT)
			{
				sprintf(input->joy_up, "Joy Left");
			}			
		}
		else
		{
			sprintf(input->joy_up, "Joy Button %i", input->ujs_id);
		}
	}
	
	else if (type == DOWN)
	{
		input->joy_down_id = joy_id;
		
		if (value != (signed)JOY_NONE)
			input->djs_id = value;
		
		input->djs_axis_id = axis;
		
		if (axis != JOY_AXIS_NONE)
		{
			if (trigger == JOY_UP)
			{
				sprintf(input->joy_down, "Joy Up");
			}
			else if (trigger == JOY_RIGHT)
			{
				sprintf(input->joy_down, "Joy Right");
			}
			else if (trigger == JOY_DOWN)
			{
				sprintf(input->joy_down, "Joy Down");
			}
			else if (trigger == JOY_LEFT)
			{
				sprintf(input->joy_down, "Joy Left");
			}			
		}
		else
		{
			sprintf(input->joy_down, "Joy Button %i", input->djs_id);
		}
		
	}
	else if (type == WEAPON)
	{
		input->joy_weap_id = joy_id;
		
		if (value != (signed)JOY_NONE)
			input->weapjs_id = value;
		
		input->weapjs_axis_id = axis;
		
		if (axis != JOY_AXIS_NONE)
		{
			if (trigger == JOY_UP)
			{
				sprintf(input->joy_weap, "Joy Up");
			}
			else if (trigger == JOY_RIGHT)
			{
				sprintf(input->joy_weap, "Joy Right");
			}
			else if (trigger == JOY_DOWN)
			{
				sprintf(input->joy_weap, "Joy Down");
			}
			else if (trigger == JOY_LEFT)
			{
				sprintf(input->joy_weap, "Joy Left");
			}			
		}
		else
		{
			sprintf(input->joy_weap, "Joy Button %i", input->weapjs_id);
		}
	}
}

void drawPinkBubbleGumRightgJoyStickConfig(void)
{	
	glTranslatef(-1.0, 15.0, -280.0);
	
	drawStringf(15.0, 5.0, "Right: %s", gPinkBubbleGumInput.joy_right);
	
	glLoadIdentity();
}

void pinkBubbleGumConfigRightJoyStickAction(void)
{
	configureJoyStick(&gPinkBubbleGumInput, RIGHT);
}

void drawPinkBubbleGumLeftgJoyStickConfig(void)
{
	glTranslatef(-1.0, 0.0, -280.0);
	
	drawStringf(15.0, 5.0, "Left: %s", gPinkBubbleGumInput.joy_left);
	
	glLoadIdentity();
}

void pinkBubbleGumConfigLeftJoyStickAction(void)
{
	configureJoyStick(&gPinkBubbleGumInput, LEFT);
}

void drawPinkBubbleGumUpgJoyStickConfig(void)
{
	glTranslatef(-1.0, -15.0, -280.0);
	
	drawStringf(15.0, 5.0, "Up: %s", gPinkBubbleGumInput.joy_up);
	
	glLoadIdentity();
}

void pinkBubbleGumConfigUpJoyStickAction(void)
{
	configureJoyStick(&gPinkBubbleGumInput, UP);
}

void drawPinkBubbleGumDowngJoyStickConfig(void)
{
	glTranslatef(-1.0, -30.0, -280.0);
	
	drawStringf(15.0, 5.0, "Down: %s", gPinkBubbleGumInput.joy_down);
	
	glLoadIdentity();
}

void pinkBubbleGumConfigDownJoyStickAction(void)
{
	configureJoyStick(&gPinkBubbleGumInput, DOWN);
}

void drawPinkBubbleGumFiregJoyStickConfig(void)
{
	glTranslatef(-1.0, -45.0, -280.0);
	
	drawStringf(15.0, 5.0, "Fire: %s", gPinkBubbleGumInput.joy_weap);
	
	glLoadIdentity();
}

void pinkBubbleGumConfigFireJoyStickAction(void)
{
	configureJoyStick(&gPinkBubbleGumInput, WEAPON);
}

// RedRover
void drawRedRoverConfigJoyStick(void)
{
	glTranslatef(-1.0, 0.0, -280.0);
	
	drawString(15.0, 5.0, "Red Rover");
	
	glLoadIdentity();
}

void redRoverConfigJoyStickAction(void)
{
	changeMenu(RIGHT);
}

void drawRedRoverRightgJoyStickConfig(void)
{	
	glTranslatef(-1.0, 15.0, -280.0);
	
	drawStringf(15.0, 5.0, "Right: %s", gRedRoverInput.joy_right);
	
	glLoadIdentity();
}

void redRoverConfigRightJoyStickAction(void)
{
	configureJoyStick(&gRedRoverInput, RIGHT);
}

void drawRedRoverLeftgJoyStickConfig(void)
{
	glTranslatef(-1.0, 0.0, -280.0);
	
	drawStringf(15.0, 5.0, "Left: %s", gRedRoverInput.joy_left);
	
	glLoadIdentity();
}

void redRoverConfigLeftJoyStickAction(void)
{
	configureJoyStick(&gRedRoverInput, LEFT);
}

void drawRedRoverUpgJoyStickConfig(void)
{
	glTranslatef(-1.0, -15.0, -280.0);
	
	drawStringf(15.0, 5.0, "Up: %s", gRedRoverInput.joy_up);
	
	glLoadIdentity();
}

void redRoverConfigUpJoyStickAction(void)
{
	configureJoyStick(&gRedRoverInput, UP);
}

void drawRedRoverDowngJoyStickConfig(void)
{
	glTranslatef(-1.0, -30.0, -280.0);
	
	drawStringf(15.0, 5.0, "Down: %s", gRedRoverInput.joy_down);
	
	glLoadIdentity();
}

void redRoverConfigDownJoyStickAction(void)
{
	configureJoyStick(&gRedRoverInput, DOWN);
}

void drawRedRoverFiregJoyStickConfig(void)
{
	glTranslatef(-1.0, -45.0, -280.0);
	
	drawStringf(15.0, 5.0, "Fire: %s", gRedRoverInput.joy_weap);
	
	glLoadIdentity();
}

void redRoverConfigFireJoyStickAction(void)
{
	configureJoyStick(&gRedRoverInput, WEAPON);
}

// GreenTree
void drawGreenTreeConfigJoyStick(void)
{
	glTranslatef(-1.0, -15.0, -280.0);
	
	drawString(15.0, 5.0, "Green Tree");
	
	glLoadIdentity();
}

void greenTreeConfigJoyStickAction(void)
{
	changeMenu(RIGHT);
}

void drawGreenTreeRightgJoyStickConfig(void)
{	
	glTranslatef(-1.0, 15.0, -280.0);
	
	drawStringf(15.0, 5.0, "Right: %s", gGreenTreeInput.joy_right);
	
	glLoadIdentity();
}

void greenTreeConfigRightJoyStickAction(void)
{
	configureJoyStick(&gGreenTreeInput, RIGHT);
}

void drawGreenTreeLeftgJoyStickConfig(void)
{
	glTranslatef(-1.0, 0.0, -280.0);
	
	drawStringf(15.0, 5.0, "Left: %s", gGreenTreeInput.joy_left);
	
	glLoadIdentity();
}

void greenTreeConfigLeftJoyStickAction(void)
{
	configureJoyStick(&gGreenTreeInput, LEFT);
}

void drawGreenTreeUpgJoyStickConfig(void)
{
	glTranslatef(-1.0, -15.0, -280.0);
	
	drawStringf(15.0, 5.0, "Up: %s", gGreenTreeInput.joy_up);
	
	glLoadIdentity();
}

void greenTreeConfigUpJoyStickAction(void)
{
	configureJoyStick(&gGreenTreeInput, UP);
}

void drawGreenTreeDowngJoyStickConfig(void)
{
	glTranslatef(-1.0, -30.0, -280.0);
	
	drawStringf(15.0, 5.0, "Down: %s", gGreenTreeInput.joy_down);
	
	glLoadIdentity();
}

void greenTreeConfigDownJoyStickAction(void)
{
	configureJoyStick(&gGreenTreeInput, DOWN);
}

void drawGreenTreeFiregJoyStickConfig(void)
{
	glTranslatef(-1.0, -45.0, -280.0);
	
	drawStringf(15.0, 5.0, "Fire: %s", gGreenTreeInput.joy_weap);
	
	glLoadIdentity();
}

void greenTreeConfigFireJoyStickAction(void)
{
	configureJoyStick(&gGreenTreeInput, WEAPON);
}

// BlueLightning
void drawBlueLightningConfigJoyStick(void)
{
	glTranslatef(-1.0, -30.0, -280.0);
	
	drawString(15.0, 5.0, "Blue Lightning");
	
	glLoadIdentity();
}

void blueLightningConfigJoyStickAction(void)
{
	changeMenu(RIGHT);
}

void drawBlueLightningRightgJoyStickConfig(void)
{	
	glTranslatef(-1.0, 15.0, -280.0);
	
	drawStringf(15.0, 5.0, "Right: %s", gBlueLightningInput.joy_right);
	
	glLoadIdentity();
}

void blueLightningConfigRightJoyStickAction(void)
{
	configureJoyStick(&gBlueLightningInput, RIGHT);
}

void drawBlueLightningLeftgJoyStickConfig(void)
{
	glTranslatef(-1.0, 0.0, -280.0);
	
	drawStringf(15.0, 5.0, "Left: %s", gBlueLightningInput.joy_left);
	
	glLoadIdentity();
}

void blueLightningConfigLeftJoyStickAction(void)
{
	configureJoyStick(&gBlueLightningInput, LEFT);
}

void drawBlueLightningUpgJoyStickConfig(void)
{
	glTranslatef(-1.0, -15.0, -280.0);
	
	drawStringf(15.0, 5.0, "Up: %s", gBlueLightningInput.joy_up);
	
	glLoadIdentity();
}

void blueLightningConfigUpJoyStickAction(void)
{
	configureJoyStick(&gBlueLightningInput, UP);
}

void drawBlueLightningDowngJoyStickConfig(void)
{
	glTranslatef(-1.0, -30.0, -280.0);
	
	drawStringf(15.0, 5.0, "Down: %s", gBlueLightningInput.joy_down);
	
	glLoadIdentity();
}

void blueLightningConfigDownJoyStickAction(void)
{
	configureJoyStick(&gBlueLightningInput, DOWN);
}

void drawBlueLightningFiregJoyStickConfig(void)
{
	glTranslatef(-1.0, -45.0, -280.0);
	
	drawStringf(15.0, 5.0, "Fire: %s", gBlueLightningInput.joy_weap);
	
	glLoadIdentity();
}

void blueLightningConfigFireJoyStickAction(void)
{
	configureJoyStick(&gBlueLightningInput, WEAPON);
}

// Audio options
void drawAudioOptionsMenu(void)
{
	glTranslatef(-1.0, -30.0, -280.0);
	
	drawString(20.0, 5.0, "Audio Options");
	
	glLoadIdentity();
}

void audioOptionsMenuAction(void)
{
	changeMenu(RIGHT);
}

// Video options
void drawVideoOptionsMenu(void)
{
	glTranslatef(-1.0, -45.0, -280.0);
	
	drawString(20.0, 5.0, "Video Options");
	
	glLoadIdentity();
}

void drawAudioEffectsOptionsMenu(void)
{
	glTranslatef(-1.0, 15.0, -280.0);
	
	if (gAudioEffectsFlag)
	{
		drawString(15.0, 5.0, "Effects: On");
	}
	else
	{
		drawString(15.0, 5.0, "Effects: Off");
	}
	
	glLoadIdentity();
}

void audioEffectsOptionsMenuAction(void)
{
	gAudioEffectsFlag = !gAudioEffectsFlag;
}

void drawAudioMusicOptionsMenu(void)
{
	glTranslatef(-1.0, 0.0, -280.0);
	
	if (gAudioMusicFlag)
	{
		drawString(15.0, 5.0, "Music: On");
	}
	else
	{
		drawString(15.0, 5.0, "Music: Off");
	}
	
	glLoadIdentity();
}

void audioMusicOptionsMenuAction(void)
{
	gAudioMusicFlag = !gAudioMusicFlag;
	if (!gAudioMusicFlag)
	{
		stopMusic();
	}
}

void videoOptionsMenuAction(void)
{
	changeMenu(RIGHT);
}

void drawRefreshRateVideoOptionMenu(void)
{
	glTranslatef(-1.0, 15.0, -280.0);
	
	if (gFpsFlag)
	{
		drawString(25.0, 5.0, "Refresh Rate: 30 FPS");
	}
	else if (gVsyncFlag)
	{
		drawString(25.0, 5.0, "Refresh Rate: VSYNC");
	}
	else /* if (!gVsyncFlag) */
	{
		drawString(25.0, 5.0, "Refresh Rate: No VSYNC");
	}
	
	glLoadIdentity();
	
	if (gDrawArrowsForRefreshRatesFlag)
	{
		glTranslatef(2.5, 1.35, -25.0);
		glColor3f(0.0, 0.0, 0.4);
		
		drawUpAndDownArrowTriangles();
		
		glLoadIdentity();
	}
}

void drawFsaaVideoOptionMenu(void)
{
	glTranslatef(-1.0, 0.0, -280.0);
	
	if (gFsaaFlag)
		drawString(25.0, 5.0, "Anti-aliasing: Yes");
	else
		drawString(25.0, 5.0, "Anti-aliasing: No");
	
	glLoadIdentity();
}

void fsaaVideoOptionMenuAction(void)
{
	int multiSampleBuffers;
	int multiSamples;
	
	gFsaaFlag = !gFsaaFlag;
	
	if (gFsaaFlag)
	{
		multiSampleBuffers = 1;
		multiSamples = 2;
	}
	else
	{
		multiSampleBuffers = 0;
		multiSamples = 0;
	}
	
	SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, multiSampleBuffers);
	SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, multiSamples);
}

void drawScreenResolutionOptionMenu(void)
{	
	glTranslatef(-1.0, -15.0, -280.0);
	
	if (gResolutions[gResolutionCounter])
	{
		drawStringf(25.0, 5.0, "Screen Resolution: %i x %i", gResolutions[gResolutionCounter]->w, gResolutions[gResolutionCounter]->h);
	}
	
	glLoadIdentity();
	
	if (gDrawArrowsForScreenResolutionsFlag)
	{
		glTranslatef(2.5, -1.35, -25.0);
		glColor3f(0.0, 0.0, 0.4);
		
		drawUpAndDownArrowTriangles();
		
		glLoadIdentity();
	}
}

void drawFullscreenVideoOptionMenu(void)
{
	glTranslatef(-1.0, -30.0, -280.0);
	
	if (gFullscreenFlag)
		drawString(25.0, 5.0, "Fullscreen: Yes");
	else
		drawString(25.0, 5.0, "Fullscreen: No");
	
	glLoadIdentity();
}

void fullscreenVideoOptionMenuAction(void)
{
	gFullscreenFlag = !gFullscreenFlag;
}

void drawQuitMenu(void)
{
	glTranslatef(-1.0, -60.0, -280.0);
	
	drawString(12.0, 5.0, "Quit");
	
	glLoadIdentity();
}

void initMenus(void)
{
	initMainMenu();
	
	Menu *playMenu =							malloc(sizeof(Menu));
	Menu *networkPlayMenu =						malloc(sizeof(Menu));
	Menu *networkServerMenu =					malloc(sizeof(Menu));
	Menu *networkServerPlayMenu =				malloc(sizeof(Menu));
	Menu *networkServerNumberOfPlayersMenu =	malloc(sizeof(Menu));
	Menu *networkServerAIModeMenu =				malloc(sizeof(Menu));
	Menu *networkServerPlayerLivesMenu =		malloc(sizeof(Menu));
	Menu *networkClientMenu =					malloc(sizeof(Menu));
	Menu *networkUserNameMenu =					malloc(sizeof(Menu));
	Menu *networkAddressFieldMenu =				malloc(sizeof(Menu));
	Menu *connectToNetworkGameMenu =			malloc(sizeof(Menu));
	Menu *gameOptionsMenu =						malloc(sizeof(Menu));
	Menu *playerOptionsMenu =					malloc(sizeof(Menu));
	Menu *pinkBubbleGumPlayerOptionsMenu =		malloc(sizeof(Menu));
	gRedRoverPlayerOptionsMenu	=				malloc(sizeof(Menu));
	gGreenTreePlayerOptionsMenu =				malloc(sizeof(Menu));
	gBlueLightningPlayerOptionsMenu =			malloc(sizeof(Menu));
	gAIModeOptionsMenu =						malloc(sizeof(Menu));
	gConfigureLivesMenu =						malloc(sizeof(Menu));
	Menu *configureKeysMenu =					malloc(sizeof(Menu));
	Menu *configureJoySticksMenu =				malloc(sizeof(Menu));
	Menu *audioOptionsMenu =					malloc(sizeof(Menu));
	Menu *audioEffectsOptionsMenu =				malloc(sizeof(Menu));
	Menu *audioMusicOptionsMenu =				malloc(sizeof(Menu));
	gVideoOptionsMenu =							malloc(sizeof(Menu));
	gRefreshRateVideoOptionMenu =				malloc(sizeof(Menu));
	Menu *fsaaVideoOptionMenu =					malloc(sizeof(Menu));
	gScreenResolutionVideoOptionMenu =			malloc(sizeof(Menu));
	Menu *fullscreenVideoOptionMenu =			malloc(sizeof(Menu));
	Menu *quitMenu =							malloc(sizeof(Menu));
	
	// set action and drawing functions
	playMenu->draw = drawPlayMenu;
	playMenu->action = initGame;
	
	networkPlayMenu->draw = drawNetworkPlayMenu;
	networkPlayMenu->action = networkPlayMenuAction;
	
	networkServerMenu->draw = drawNetworkServerMenu;
	networkServerMenu->action = networkServerMenuAction;
	
	networkServerPlayMenu->draw = drawNetworkServerPlayMenu;
	networkServerPlayMenu->action = networkServerPlayMenuAction;
	
	networkServerNumberOfPlayersMenu->draw = drawNetworkServerNumberOfPlayersMenu;
	networkServerNumberOfPlayersMenu->action = networkServerNumberOfPlayersMenuAction;
	
	networkServerAIModeMenu->draw = drawNetworkServerAIModeMenu;
	networkServerAIModeMenu->action = networkServerAIModeMenuAction;
	gAINetModeOptionsMenu = networkServerAIModeMenu;
	
	networkServerPlayerLivesMenu->draw = drawNetworkServerPlayerLivesMenu;
	networkServerPlayerLivesMenu->action = networkServerPlayerLivesMenuAction;
	
	networkClientMenu->draw = drawNetworkClientMenu;
	networkClientMenu->action = networkClientMenuAction;
	
	networkUserNameMenu->draw = drawNetworkUserNameFieldMenu;
	networkUserNameMenu->action = networkUserNameFieldMenuAction;
	
	networkAddressFieldMenu->draw = drawNetworkAddressFieldMenu;
	networkAddressFieldMenu->action = networkAddressFieldMenuAction;
	
	connectToNetworkGameMenu->draw = drawConnectToNetworkGameMenu;
	connectToNetworkGameMenu->action = connectToNetworkGameMenuAction;
	
	gameOptionsMenu->draw = drawGameOptionsMenu;
	gameOptionsMenu->action = gameOptionsMenuAction;
	
	playerOptionsMenu->draw = drawPlayerOptionsMenu;
	playerOptionsMenu->action = playerOptionsMenuAction;
	
	pinkBubbleGumPlayerOptionsMenu->draw = drawPinkBubbleGumPlayerOptionsMenu;
	pinkBubbleGumPlayerOptionsMenu->action = pinkBubbleGumPlayerOptionsMenuAction;
	
	gRedRoverPlayerOptionsMenu->draw = drawRedRoverPlayerOptionsMenu;
	gRedRoverPlayerOptionsMenu->action = redRoverPlayerOptionsMenuAction;
	
	gGreenTreePlayerOptionsMenu->draw = drawGreenTreePlayerOptionsMenu;
	gGreenTreePlayerOptionsMenu->action = greenTreePlayerOptionsMenuAction;
	
	gBlueLightningPlayerOptionsMenu->draw = drawBlueLightningPlayerOptionsMenu;
	gBlueLightningPlayerOptionsMenu->action = blueLightningPlayerOptionsMenuAction;
	
	gAIModeOptionsMenu->draw = drawAIModeOptionsMenu;
	gAIModeOptionsMenu->action = NULL;
	
	gConfigureLivesMenu->draw = drawConfigureLivesMenu;
	gConfigureLivesMenu->action = NULL;
	
	configureKeysMenu->draw = drawConfigureKeysMenu;
	configureKeysMenu->action = configureKeysMenuAction;
	
	configureJoySticksMenu->draw = drawJoySticksConfigureMenu;
	configureJoySticksMenu->action = joySticksConfigureMenuAction;
	
	audioOptionsMenu->draw = drawAudioOptionsMenu;
	audioOptionsMenu->action = audioOptionsMenuAction;
	
	audioEffectsOptionsMenu->draw = drawAudioEffectsOptionsMenu;
	audioEffectsOptionsMenu->action = audioEffectsOptionsMenuAction;
	
	audioMusicOptionsMenu->draw = drawAudioMusicOptionsMenu;
	audioMusicOptionsMenu->action = audioMusicOptionsMenuAction;
	
	gVideoOptionsMenu->draw = drawVideoOptionsMenu;
	gVideoOptionsMenu->action = videoOptionsMenuAction;
	
	gRefreshRateVideoOptionMenu->draw = drawRefreshRateVideoOptionMenu;
	gRefreshRateVideoOptionMenu->action = NULL;
	
	fsaaVideoOptionMenu->draw = drawFsaaVideoOptionMenu;
	fsaaVideoOptionMenu->action = fsaaVideoOptionMenuAction;
	
	gScreenResolutionVideoOptionMenu->draw = drawScreenResolutionOptionMenu;
	gScreenResolutionVideoOptionMenu->action = NULL;
	
	fullscreenVideoOptionMenu->draw = drawFullscreenVideoOptionMenu;
	fullscreenVideoOptionMenu->action = fullscreenVideoOptionMenuAction;
	
	quitMenu->draw = drawQuitMenu;
	quitMenu->action = SDL_Terminate;
	
	// character config menu keys
	gCharacterConfigureKeys[0][0].draw = drawPinkBubbleGumConfigKey;
	gCharacterConfigureKeys[0][0].action = pinkBubbleGumKeyMenuAction;
	
	gCharacterConfigureKeys[1][0].draw = drawRedRoverConfigKey;
	gCharacterConfigureKeys[1][0].action = redRoverKeyMenuAction;
	
	gCharacterConfigureKeys[2][0].draw = drawGreenTreeConfigKey;
	gCharacterConfigureKeys[2][0].action = greenTreeKeyMenuAction;
	
	gCharacterConfigureKeys[3][0].draw = drawBlueLightningConfigKey;
	gCharacterConfigureKeys[3][0].action = blueLightningKeyMenuAction;
	
	// pinkBubbleGum configs
	gCharacterConfigureKeys[0][1].draw = drawPinkBubbleGumConfigRightKey;
	gCharacterConfigureKeys[0][1].action = pinkBubbleGumRightKeyMenuAction;
	
	gCharacterConfigureKeys[0][2].draw = drawPinkBubbleGumConfigLeftKey;
	gCharacterConfigureKeys[0][2].action = pinkBubbleGumLeftKeyMenuAction;
	
	gCharacterConfigureKeys[0][3].draw = drawPinkBubbleGumConfigUpKey;
	gCharacterConfigureKeys[0][3].action = pinkBubbleGumUpKeyMenuAction;
	
	gCharacterConfigureKeys[0][4].draw = drawPinkBubbleGumConfigDownKey;
	gCharacterConfigureKeys[0][4].action = pinkBubbleGumDownKeyMenuAction;
	
	gCharacterConfigureKeys[0][5].draw = drawPinkBubbleGumConfigFireKey;
	gCharacterConfigureKeys[0][5].action = pinkBubbleGumFireKeyMenuAction;
	
	// redRover configs
	gCharacterConfigureKeys[1][1].draw = drawRedRoverConfigRightKey;
	gCharacterConfigureKeys[1][1].action = redRoverRightKeyMenuAction;
	
	gCharacterConfigureKeys[1][2].draw = drawRedRoverConfigLeftKey;
	gCharacterConfigureKeys[1][2].action = redRoverLeftKeyMenuAction;
	
	gCharacterConfigureKeys[1][3].draw = drawRedRoverConfigUpKey;
	gCharacterConfigureKeys[1][3].action = redRoverUpKeyMenuAction;
	
	gCharacterConfigureKeys[1][4].draw = drawRedRoverConfigDownKey;
	gCharacterConfigureKeys[1][4].action = redRoverDownKeyMenuAction;
	
	gCharacterConfigureKeys[1][5].draw = drawRedRoverConfigFireKey;
	gCharacterConfigureKeys[1][5].action = redRoverFireKeyMenuAction;
	
	// GreenTree configs
	gCharacterConfigureKeys[2][1].draw = drawGreenTreeConfigRightKey;
	gCharacterConfigureKeys[2][1].action = greenTreeRightKeyMenuAction;
	
	gCharacterConfigureKeys[2][2].draw = drawGreenTreeConfigLeftKey;
	gCharacterConfigureKeys[2][2].action = greenTreeLeftKeyMenuAction;
	
	gCharacterConfigureKeys[2][3].draw = drawGreenTreeConfigUpKey;
	gCharacterConfigureKeys[2][3].action = greenTreeUpKeyMenuAction;
	
	gCharacterConfigureKeys[2][4].draw = drawGreenTreeConfigDownKey;
	gCharacterConfigureKeys[2][4].action = greenTreeDownKeyMenuAction;
	
	gCharacterConfigureKeys[2][5].draw = drawGreenTreeConfigFireKey;
	gCharacterConfigureKeys[2][5].action = greenTreeFireKeyMenuAction;
	
	// blueLightning configs
	gCharacterConfigureKeys[3][1].draw = drawBlueLightningConfigRightKey;
	gCharacterConfigureKeys[3][1].action = blueLightningRightKeyMenuAction;
	
	gCharacterConfigureKeys[3][2].draw = drawBlueLightningConfigLeftKey;
	gCharacterConfigureKeys[3][2].action = blueLightningLeftKeyMenuAction;
	
	gCharacterConfigureKeys[3][3].draw = drawBlueLightningConfigUpKey;
	gCharacterConfigureKeys[3][3].action = blueLightningUpKeyMenuAction;
	
	gCharacterConfigureKeys[3][4].draw = drawBlueLightningConfigDownKey;
	gCharacterConfigureKeys[3][4].action = blueLightningDownKeyMenuAction;
	
	gCharacterConfigureKeys[3][5].draw = drawBlueLightningConfigFireKey;
	gCharacterConfigureKeys[3][5].action = blueLightningFireKeyMenuAction;
	
	// Joystick menus
	
	// PinkBubbleGum configs
	gJoyStickConfig[0][0].draw = drawPinkBubbleGumConfigJoyStick;
	gJoyStickConfig[0][0].action = pinkBubbleGumConfigJoyStickAction;
	
	gJoyStickConfig[0][1].draw = drawPinkBubbleGumRightgJoyStickConfig;
	gJoyStickConfig[0][1].action = pinkBubbleGumConfigRightJoyStickAction;
	
	gJoyStickConfig[0][2].draw = drawPinkBubbleGumLeftgJoyStickConfig;
	gJoyStickConfig[0][2].action = pinkBubbleGumConfigLeftJoyStickAction;
	
	gJoyStickConfig[0][3].draw = drawPinkBubbleGumUpgJoyStickConfig;
	gJoyStickConfig[0][3].action = pinkBubbleGumConfigUpJoyStickAction;
	
	gJoyStickConfig[0][4].draw = drawPinkBubbleGumDowngJoyStickConfig;
	gJoyStickConfig[0][4].action = pinkBubbleGumConfigDownJoyStickAction;
	
	gJoyStickConfig[0][5].draw = drawPinkBubbleGumFiregJoyStickConfig;
	gJoyStickConfig[0][5].action = pinkBubbleGumConfigFireJoyStickAction;
	
	// RedRover configs
	gJoyStickConfig[1][0].draw = drawRedRoverConfigJoyStick;
	gJoyStickConfig[1][0].action = redRoverConfigJoyStickAction;
	
	gJoyStickConfig[1][1].draw = drawRedRoverRightgJoyStickConfig;
	gJoyStickConfig[1][1].action = redRoverConfigRightJoyStickAction;
	
	gJoyStickConfig[1][2].draw = drawRedRoverLeftgJoyStickConfig;
	gJoyStickConfig[1][2].action = redRoverConfigLeftJoyStickAction;
	
	gJoyStickConfig[1][3].draw = drawRedRoverUpgJoyStickConfig;
	gJoyStickConfig[1][3].action = redRoverConfigUpJoyStickAction;
	
	gJoyStickConfig[1][4].draw = drawRedRoverDowngJoyStickConfig;
	gJoyStickConfig[1][4].action = redRoverConfigDownJoyStickAction;
	
	gJoyStickConfig[1][5].draw = drawRedRoverFiregJoyStickConfig;
	gJoyStickConfig[1][5].action = redRoverConfigFireJoyStickAction;
	
	// GreenTree configs
	gJoyStickConfig[2][0].draw = drawGreenTreeConfigJoyStick;
	gJoyStickConfig[2][0].action = greenTreeConfigJoyStickAction;
	
	gJoyStickConfig[2][1].draw = drawGreenTreeRightgJoyStickConfig;
	gJoyStickConfig[2][1].action = greenTreeConfigRightJoyStickAction;
	
	gJoyStickConfig[2][2].draw = drawGreenTreeLeftgJoyStickConfig;
	gJoyStickConfig[2][2].action = greenTreeConfigLeftJoyStickAction;
	
	gJoyStickConfig[2][3].draw = drawGreenTreeUpgJoyStickConfig;
	gJoyStickConfig[2][3].action = greenTreeConfigUpJoyStickAction;
	
	gJoyStickConfig[2][4].draw = drawGreenTreeDowngJoyStickConfig;
	gJoyStickConfig[2][4].action = greenTreeConfigDownJoyStickAction;
	
	gJoyStickConfig[2][5].draw = drawGreenTreeFiregJoyStickConfig;
	gJoyStickConfig[2][5].action = greenTreeConfigFireJoyStickAction;
	
	// BlueLightning configs
	gJoyStickConfig[3][0].draw = drawBlueLightningConfigJoyStick;
	gJoyStickConfig[3][0].action = blueLightningConfigJoyStickAction;
	
	gJoyStickConfig[3][1].draw = drawBlueLightningRightgJoyStickConfig;
	gJoyStickConfig[3][1].action = blueLightningConfigRightJoyStickAction;
	
	gJoyStickConfig[3][2].draw = drawBlueLightningLeftgJoyStickConfig;
	gJoyStickConfig[3][2].action = blueLightningConfigLeftJoyStickAction;
	
	gJoyStickConfig[3][3].draw = drawBlueLightningUpgJoyStickConfig;
	gJoyStickConfig[3][3].action = blueLightningConfigUpJoyStickAction;
	
	gJoyStickConfig[3][4].draw = drawBlueLightningDowngJoyStickConfig;
	gJoyStickConfig[3][4].action = blueLightningConfigDownJoyStickAction;
	
	gJoyStickConfig[3][5].draw = drawBlueLightningFiregJoyStickConfig;
	gJoyStickConfig[3][5].action = blueLightningConfigFireJoyStickAction;
	
	// Add Menus
	addSubMenu(&gMainMenu, playMenu);
	addSubMenu(&gMainMenu, networkPlayMenu);
	addSubMenu(&gMainMenu, gameOptionsMenu);
	addSubMenu(&gMainMenu, audioOptionsMenu);
	addSubMenu(&gMainMenu, gVideoOptionsMenu);
	addSubMenu(&gMainMenu, quitMenu);
	
	addSubMenu(networkPlayMenu, networkServerMenu);
	addSubMenu(networkPlayMenu, networkClientMenu);
	addSubMenu(networkPlayMenu, networkUserNameMenu);
	
	addSubMenu(networkServerMenu, networkServerPlayMenu);
	addSubMenu(networkServerMenu, networkServerNumberOfPlayersMenu);
	addSubMenu(networkServerMenu, networkServerAIModeMenu);
	addSubMenu(networkServerMenu, networkServerPlayerLivesMenu);
	
	addSubMenu(networkClientMenu, networkAddressFieldMenu);
	addSubMenu(networkClientMenu, connectToNetworkGameMenu);
	
	addSubMenu(gameOptionsMenu, playerOptionsMenu);
	addSubMenu(playerOptionsMenu, pinkBubbleGumPlayerOptionsMenu);
	addSubMenu(playerOptionsMenu, gRedRoverPlayerOptionsMenu);
	addSubMenu(playerOptionsMenu, gGreenTreePlayerOptionsMenu);
	addSubMenu(playerOptionsMenu, gBlueLightningPlayerOptionsMenu);
	addSubMenu(playerOptionsMenu, gAIModeOptionsMenu);
	addSubMenu(playerOptionsMenu, gConfigureLivesMenu);
	
	addSubMenu(gameOptionsMenu, configureKeysMenu);
	addSubMenu(gameOptionsMenu, configureJoySticksMenu);
	
	addSubMenu(audioOptionsMenu, audioEffectsOptionsMenu);
	addSubMenu(audioOptionsMenu, audioMusicOptionsMenu);
	
	addSubMenu(gVideoOptionsMenu, gRefreshRateVideoOptionMenu);
	addSubMenu(gVideoOptionsMenu, fsaaVideoOptionMenu);
	addSubMenu(gVideoOptionsMenu, gScreenResolutionVideoOptionMenu);
	addSubMenu(gVideoOptionsMenu, fullscreenVideoOptionMenu);
	
	// characters key config menu
	addSubMenu(configureKeysMenu, &gCharacterConfigureKeys[0][0]);
	addSubMenu(configureKeysMenu, &gCharacterConfigureKeys[1][0]);
	addSubMenu(configureKeysMenu, &gCharacterConfigureKeys[2][0]);
	addSubMenu(configureKeysMenu, &gCharacterConfigureKeys[3][0]);
	
	// characters joy stick config menu
	addSubMenu(configureJoySticksMenu, &gJoyStickConfig[0][0]);
	addSubMenu(configureJoySticksMenu, &gJoyStickConfig[1][0]);
	addSubMenu(configureJoySticksMenu, &gJoyStickConfig[2][0]);
	addSubMenu(configureJoySticksMenu, &gJoyStickConfig[3][0]);
	
	// keys config sub menus
	
	// pinkBubbleGum
	addSubMenu(&gCharacterConfigureKeys[0][0], &gCharacterConfigureKeys[0][1]);
	addSubMenu(&gCharacterConfigureKeys[0][0], &gCharacterConfigureKeys[0][2]);
	addSubMenu(&gCharacterConfigureKeys[0][0], &gCharacterConfigureKeys[0][3]);
	addSubMenu(&gCharacterConfigureKeys[0][0], &gCharacterConfigureKeys[0][4]);
	addSubMenu(&gCharacterConfigureKeys[0][0], &gCharacterConfigureKeys[0][5]);
	
	// redRover
	addSubMenu(&gCharacterConfigureKeys[1][0], &gCharacterConfigureKeys[1][1]);
	addSubMenu(&gCharacterConfigureKeys[1][0], &gCharacterConfigureKeys[1][2]);
	addSubMenu(&gCharacterConfigureKeys[1][0], &gCharacterConfigureKeys[1][3]);
	addSubMenu(&gCharacterConfigureKeys[1][0], &gCharacterConfigureKeys[1][4]);
	addSubMenu(&gCharacterConfigureKeys[1][0], &gCharacterConfigureKeys[1][5]);
	
	// greenTree
	addSubMenu(&gCharacterConfigureKeys[2][0], &gCharacterConfigureKeys[2][1]);
	addSubMenu(&gCharacterConfigureKeys[2][0], &gCharacterConfigureKeys[2][2]);
	addSubMenu(&gCharacterConfigureKeys[2][0], &gCharacterConfigureKeys[2][3]);
	addSubMenu(&gCharacterConfigureKeys[2][0], &gCharacterConfigureKeys[2][4]);
	addSubMenu(&gCharacterConfigureKeys[2][0], &gCharacterConfigureKeys[2][5]);
	
	// blueLightning
	addSubMenu(&gCharacterConfigureKeys[3][0], &gCharacterConfigureKeys[3][1]);
	addSubMenu(&gCharacterConfigureKeys[3][0], &gCharacterConfigureKeys[3][2]);
	addSubMenu(&gCharacterConfigureKeys[3][0], &gCharacterConfigureKeys[3][3]);
	addSubMenu(&gCharacterConfigureKeys[3][0], &gCharacterConfigureKeys[3][4]);
	addSubMenu(&gCharacterConfigureKeys[3][0], &gCharacterConfigureKeys[3][5]);
	
	// joy stick config sub menus
	
	// PinkBubbleGum
	addSubMenu(&gJoyStickConfig[0][0], &gJoyStickConfig[0][1]);
	addSubMenu(&gJoyStickConfig[0][0], &gJoyStickConfig[0][2]);
	addSubMenu(&gJoyStickConfig[0][0], &gJoyStickConfig[0][3]);
	addSubMenu(&gJoyStickConfig[0][0], &gJoyStickConfig[0][4]);
	addSubMenu(&gJoyStickConfig[0][0], &gJoyStickConfig[0][5]);
	
	// RedRover
	addSubMenu(&gJoyStickConfig[1][0], &gJoyStickConfig[1][1]);
	addSubMenu(&gJoyStickConfig[1][0], &gJoyStickConfig[1][2]);
	addSubMenu(&gJoyStickConfig[1][0], &gJoyStickConfig[1][3]);
	addSubMenu(&gJoyStickConfig[1][0], &gJoyStickConfig[1][4]);
	addSubMenu(&gJoyStickConfig[1][0], &gJoyStickConfig[1][5]);
	
	// GreenTree
	addSubMenu(&gJoyStickConfig[2][0], &gJoyStickConfig[2][1]);
	addSubMenu(&gJoyStickConfig[2][0], &gJoyStickConfig[2][2]);
	addSubMenu(&gJoyStickConfig[2][0], &gJoyStickConfig[2][3]);
	addSubMenu(&gJoyStickConfig[2][0], &gJoyStickConfig[2][4]);
	addSubMenu(&gJoyStickConfig[2][0], &gJoyStickConfig[2][5]);
	
	// BlueLightning
	addSubMenu(&gJoyStickConfig[3][0], &gJoyStickConfig[3][1]);
	addSubMenu(&gJoyStickConfig[3][0], &gJoyStickConfig[3][2]);
	addSubMenu(&gJoyStickConfig[3][0], &gJoyStickConfig[3][3]);
	addSubMenu(&gJoyStickConfig[3][0], &gJoyStickConfig[3][4]);
	addSubMenu(&gJoyStickConfig[3][0], &gJoyStickConfig[3][5]);
}

static char *convertKeyCodeToString(unsigned theKeyCode)
{
	switch (theKeyCode)
	{
		case SDLK_RIGHT:
			sprintf(gKeyCode, "right arrow");
			break;
		case SDLK_LEFT:
			sprintf(gKeyCode, "left arrow");
			break;
		case SDLK_UP:
			sprintf(gKeyCode, "up arrow");
			break;
		case SDLK_DOWN:
			sprintf(gKeyCode, "down arrow");
			break;
		case SDLK_SPACE:
			sprintf(gKeyCode, "spacebar");
			break;
		case SDLK_INSERT:
			sprintf(gKeyCode, "insert");
			break;
		case SDLK_HOME:
			sprintf(gKeyCode, "home");
			break;
		case SDLK_END:
			sprintf(gKeyCode, "end");
			break;
		case SDLK_PAGEUP:
			sprintf(gKeyCode, "pageup");
			break;
		case SDLK_PAGEDOWN:
			sprintf(gKeyCode, "pagedown");
			break;
		case SDLK_RSHIFT:
		case SDLK_LSHIFT:
			sprintf(gKeyCode, "shift");
			break;
		case SDLK_BACKSPACE:
			sprintf(gKeyCode, "backspace");
			break;
		case SDLK_TAB:
			sprintf(gKeyCode, "tab");
			break;
		case SDLK_F1:
			sprintf(gKeyCode, "F1");
			break;
		case SDLK_F2:
			sprintf(gKeyCode, "F2");
			break;
		case SDLK_F3:
			sprintf(gKeyCode, "F3");
			break;
		case SDLK_F4:
			sprintf(gKeyCode, "F4");
			break;
		case SDLK_F5:
			sprintf(gKeyCode, "F5");
			break;
		case SDLK_F6:
			sprintf(gKeyCode, "F6");
			break;
		case SDLK_F7:
			sprintf(gKeyCode, "F7");
			break;
		case SDLK_F8:
			sprintf(gKeyCode, "F8");
			break;
		case SDLK_F9:
			sprintf(gKeyCode, "F9");
			break;
		case SDLK_F10:
			sprintf(gKeyCode, "F10");
			break;
		case SDLK_F11:
			sprintf(gKeyCode, "F11");
			break;
		case SDLK_F12:
			sprintf(gKeyCode, "F12");
			break;
		case SDLK_F13:
			sprintf(gKeyCode, "F13");
			break;
		case SDLK_F14:
			sprintf(gKeyCode, "F14");
			break;
		case SDLK_F15:
			sprintf(gKeyCode, "F15");
			break;
		case SDLK_CAPSLOCK:
			sprintf(gKeyCode, "capslock");
			break;
		case SDLK_NUMLOCK:
			sprintf(gKeyCode, "numlock");
			break;
		case SDLK_SCROLLOCK:
			sprintf(gKeyCode, "scrollock");
			break;
		case SDLK_RCTRL:
		case SDLK_LCTRL:
			sprintf(gKeyCode, "control");
			break;
		case SDLK_RALT:
		case SDLK_LALT:
			sprintf(gKeyCode, "alt");
			break;
		case SDLK_RMETA:
		case SDLK_LMETA:
			sprintf(gKeyCode, "meta/command");
			break;
		case SDLK_KP0:
			sprintf(gKeyCode, "keypad 0");
			break;
		case SDLK_KP1:
			sprintf(gKeyCode, "keypad 1");
			break;
		case SDLK_KP2:
			sprintf(gKeyCode, "keypad 2");
			break;
		case SDLK_KP3:
			sprintf(gKeyCode, "keypad 3");
			break;
		case SDLK_KP4:
			sprintf(gKeyCode, "keypad 4");
			break;
		case SDLK_KP5:
			sprintf(gKeyCode, "keypad 5");
			break;
		case SDLK_KP6:
			sprintf(gKeyCode, "keypad 6");
			break;
		case SDLK_KP7:
			sprintf(gKeyCode, "keypad 7");
			break;
		case SDLK_KP8:
			sprintf(gKeyCode, "keypad 8");
			break;
		case SDLK_KP9:
			sprintf(gKeyCode, "keypad 9");
			break;
		case SDLK_KP_PERIOD:
			sprintf(gKeyCode, "keypad .");
			break;
		case SDLK_KP_DIVIDE:
			sprintf(gKeyCode, "keypad /");
			break;
		case SDLK_KP_MULTIPLY:
			sprintf(gKeyCode, "keypad *");
			break;
		case SDLK_KP_MINUS:
			sprintf(gKeyCode, "keypad -");
			break;
		case SDLK_KP_PLUS:
			sprintf(gKeyCode, "keypad +");
			break;
		case SDLK_KP_ENTER:
			sprintf(gKeyCode, "keypad enter");
			break;
		case SDLK_KP_EQUALS:
			sprintf(gKeyCode, "keypad =");
			break;
			
		default:
			sprintf(gKeyCode, "%c", theKeyCode);
	}
	
	return gKeyCode;
}

static unsigned getKey(void)
{
	SDL_Event event;
	SDL_bool quit = SDL_FALSE;
	unsigned key = 0;
	
	while (!quit)
	{
		while (SDL_PollEvent(&event))
		{
			switch (event.type)
			{
				case SDL_KEYDOWN:
					
					if (event.key.keysym.sym != SDLK_RETURN && event.key.keysym.sym != SDLK_ESCAPE)
					{
						key = event.key.keysym.sym;
					}
					
					quit = SDL_TRUE;
					break;
			}
		}
	}
	
	return key;
}

unsigned getJoyStickTrigger(Sint16 *value, Uint8 *axis, int *joy_id)
{
	SDL_Event event;
	unsigned trigger = JOY_NONE;
	
	*axis = JOY_AXIS_NONE;
	*value = JOY_NONE;
	
	/*
	 * When we are finished, end the execution of the function by returning the trigger.
	 * The reason why we end the function instead of waiting for the loop to end with a boolean is that sometimes event.type can be both SDL_JOYAXISMOTION and SDL_JOYBUTTONDOWN when the user configures their controller
	 */
	while (SDL_TRUE)
	{
		while (SDL_PollEvent(&event))
		{
			switch (event.type)
			{
				case SDL_KEYDOWN:
					if (event.key.keysym.sym == SDLK_ESCAPE)
						return trigger;
					
					break;
				case SDL_JOYBUTTONDOWN:
					*value = event.jbutton.button;
					
					*joy_id = event.jbutton.which;
					
					return trigger;
					
					case SDL_JOYAXISMOTION:
					
					// check for invalid value
					if (!(( event.jaxis.value < -32000 ) || (event.jaxis.value > 32000 )))
					{
						zgPrint("Invalid value: %i with axis: %i", event.jaxis.value, event.jaxis.axis);
						break;
					}
					
					// x axis
					if (event.jaxis.axis == 0)
					{
						if (event.jaxis.value > 32000)
							trigger = JOY_RIGHT;
						else if (event.jaxis.value < -32000)
							trigger = JOY_LEFT;
					}
					// y axis
					else if (event.jaxis.axis == 1)
					{
						if (event.jaxis.value > 0)
							trigger = JOY_DOWN;
						else if (event.jaxis.value < 0)
							trigger = JOY_UP;
					}
					
					*joy_id = event.jaxis.which;
					*value = event.jaxis.value;
					*axis = event.jaxis.axis;
					
					return trigger;
					break;
			}
		}
	}
	
	return trigger;
}
