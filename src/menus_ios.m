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

#include "menus.h"
#include "window.h"
#include "characters.h"
#include "audio.h"
#include "menu_actions.h"

#import <UIKit/UIKit.h>
#import "SkyCheckers-Swift.h"

static UIViewController *gMainMenuController;
static UIViewController *gPauseMenuController;

static UIView *gCurrentMenuView;

static void hideGameMenus(ZGWindow *windowRef);

static UIView *metalViewForWindow(ZGWindow *windowRef)
{
	UIWindow *window = (__bridge UIWindow *)(ZGWindowHandle(windowRef));
	UIView *rootView = window.rootViewController.view;
	UIView *metalView = [[rootView subviews] firstObject];
	return metalView;
}

static void playGame(ZGWindow *window, bool tutorial)
{
	hideGameMenus(window);
	
	if (tutorial)
	{
		playTutorial(window);
	}
	else
	{
		initGame(window, true, false);
	}
}

@interface MainMenuHandler : NSObject

@property (nonatomic) ZGWindow *window;

@end

@implementation MainMenuHandler

- (void)playGame
{
#if PLATFORM_TVOS
	if (gSuggestedTutorial || !willUseSiriRemote())
#else
	if (gSuggestedTutorial)
#endif
	{
		playGame(_window, false);
	}
	else
	{
		gSuggestedTutorial = true;
		
#if PLATFORM_TVOS
		NSString *message = @"Do you want to play the tutorial to learn the Siri Remote controls?";
#else
		NSString *message = @"Do you want to play the tutorial to learn the touch controls?";
#endif
		
		UIAlertController *alertController = [UIAlertController alertControllerWithTitle:@"How to Play" message:message preferredStyle:UIAlertControllerStyleAlert];
		
		[alertController addAction:[UIAlertAction actionWithTitle:@"Play Tutorial" style:UIAlertActionStyleDefault handler:^(UIAlertAction * _Nonnull action) {
			dispatch_async(dispatch_get_main_queue(), ^{
				playGame(self->_window, true);
			});
		}]];
		
		[alertController addAction:[UIAlertAction actionWithTitle:@"Skip" style:UIAlertActionStyleCancel handler:^(UIAlertAction * _Nonnull action) {
			dispatch_async(dispatch_get_main_queue(), ^{
				playGame(self->_window, false);
			});
		}]];
		
		UIWindow *window = (__bridge UIWindow *)(ZGWindowHandle(_window));
		[window.rootViewController presentViewController:alertController animated:YES completion:nil];
	}
}

- (void)playTutorial
{
#if PLATFORM_TVOS
	if (!gSuggestedTutorial && willUseSiriRemote())
	{
		gSuggestedTutorial = true;
	}
#endif
	playGame(_window, true);
}

@end

@interface PauseMenuHandler : NSObject
{
@public
	GameState _resumedGameState;
}

@property (nonatomic) ZGWindow *window;
@property (nonatomic) GameState *gameState;
@property (nonatomic) void (*exitGameFunc)(ZGWindow *);

@end

@implementation PauseMenuHandler

- (void)resumeGame
{
#if PLATFORM_TVOS
	ZGInstallMenuGesture(_window);
#else
	ZGInstallTouchGestures(_window);
#endif
	[gPauseMenuController.view removeFromSuperview];
	
	UIView *metalView = metalViewForWindow(_window);
	((CAMetalLayer *)metalView.layer).presentsWithTransaction = NO;
	
	resumeGame(_resumedGameState, _gameState);
}

- (void)exitGame
{
	UIView *metalView = metalViewForWindow(_window);
	
	[gPauseMenuController.view removeFromSuperview];
	gCurrentMenuView.frame = metalView.bounds;
	[metalView addSubview:gCurrentMenuView];
	
	unPauseMusic();
	
	*_gameState = _resumedGameState;
	
	if (_exitGameFunc != NULL)
	{
		_exitGameFunc(_window);
	}
}

@end

static uint8_t numberOfHumanPlayers(void)
{
	uint8_t numberOfHumans = 0;
	if (gPinkBubbleGum.state == CHARACTER_HUMAN_STATE)
	{
		numberOfHumans++;
	}
	else
	{
		return numberOfHumans;
	}
	
	if (gRedRover.state == CHARACTER_HUMAN_STATE)
	{
		numberOfHumans++;
	}
	else
	{
		return numberOfHumans;
	}
	
	if (gGreenTree.state == CHARACTER_HUMAN_STATE)
	{
		numberOfHumans++;
	}
	else
	{
		return numberOfHumans;
	}
	
	if (gBlueLightning.state == CHARACTER_HUMAN_STATE)
	{
		numberOfHumans++;
	}
	
	return numberOfHumans;
}

static uint8_t currentAIModeIndex(void)
{
	if (gAIMode == AI_EASY_MODE)
	{
		return 0;
	}
	else if (gAIMode == AI_MEDIUM_MODE)
	{
		return 1;
	}
	else if (gAIMode == AI_HARD_MODE)
	{
		return 2;
	}
	else
	{
		gAIMode = AI_EASY_MODE;
	}
	return 0;
}

static MainMenuHandler *gMainMenuHandler;
static PauseMenuHandler *gPauseMenuHandler;

static UIViewController *makeMainMenuController(UIView *metalView, GameState *gameState)
{
	NSString *initialOnlineName = [NSString stringWithCString:gUserNameString encoding:NSUTF8StringEncoding] ?: @"";
	NSString *initialHostAddress = [NSString stringWithCString:gServerAddressString encoding:NSUTF8StringEncoding] ?: @"";
	UIViewController *viewController = [MainMenuController viewControllerWithOnPlay:^{
		[gMainMenuHandler playGame];
	} onTutorial:^{
		[gMainMenuHandler playTutorial];
	} initialOnlineName:initialOnlineName onOnlineNameChange:^(NSString *name) {
		const char *utf8 = name.UTF8String;
		if (utf8 != NULL)
		{
			strncpy(gUserNameString, utf8, MAX_USER_NAME_SIZE - 1);
			gUserNameString[MAX_USER_NAME_SIZE - 1] = '\0';
		}
		else
		{
			gUserNameString[0] = '\0';
		}
		gUserNameStringIndex = (int)strlen(gUserNameString);
	} initialFriendsJoining:gNumberOfNetHumans onFriendsJoiningChange:^(NSInteger value) {
		gNumberOfNetHumans = (int)value;
	} initialHostAddress:initialHostAddress onHostAddressChange:^(NSString *address) {
		const char *utf8 = address.UTF8String;
		if (utf8 != NULL)
		{
			strncpy(gServerAddressString, utf8, MAX_SERVER_ADDRESS_SIZE - 1);
			gServerAddressString[MAX_SERVER_ADDRESS_SIZE - 1] = '\0';
		}
		else
		{
			gServerAddressString[0] = '\0';
		}
		gServerAddressStringIndex = (int)strlen(gServerAddressString);
	} onStartGame:^{
		if (startNetworkGame(gMainMenuHandler.window))
		{
			hideGameMenus(gMainMenuHandler.window);
			gCurrentMenuView = gMainMenuController.view;
		}
	} onConnect:^{
		if (connectToNetworkGame(gameState))
		{
			hideGameMenus(gMainMenuHandler.window);
		}
	} initialHumanPlayers:numberOfHumanPlayers() onHumanPlayersChange:^(NSInteger count) {
		gPinkBubbleGum.state = count >= 1 ? CHARACTER_HUMAN_STATE : CHARACTER_AI_STATE;
		gRedRover.state = count >= 2 ? CHARACTER_HUMAN_STATE : CHARACTER_AI_STATE;
		gGreenTree.state = count >= 3 ? CHARACTER_HUMAN_STATE : CHARACTER_AI_STATE;
		gBlueLightning.state = count >= 4 ? CHARACTER_HUMAN_STATE : CHARACTER_AI_STATE;
	} initialPlayerLives:gCharacterLives onPlayerLivesChange:^(NSInteger lives) {
		gCharacterLives = (int)lives;
	} initialAIDifficulty:currentAIModeIndex() onAIDifficultyChange:^(NSInteger index) {
		if (index == 0) gAIMode = AI_EASY_MODE;
		else if (index == 1) gAIMode = AI_MEDIUM_MODE;
		else gAIMode = AI_HARD_MODE;
	} initialEffectsEnabled:gAudioEffectsFlag onEffectsChange:^(BOOL enabled) {
		gAudioEffectsFlag = enabled;
	} initialMusicEnabled:gAudioMusicFlag onMusicChange:^(BOOL enabled) {
		updateAudioMusic(gMainMenuHandler.window, enabled);
	}];
	viewController.view.frame = metalView.bounds;
	viewController.view.autoresizingMask = UIViewAutoresizingFlexibleWidth | UIViewAutoresizingFlexibleHeight;
	viewController.view.backgroundColor = UIColor.clearColor;

	return viewController;
}

static UIViewController *makePauseMenuController(UIView *metalView)
{
	UIViewController *viewController = [PauseMenuController viewControllerWithOnResume:^{
		[gPauseMenuHandler resumeGame];
	} onExit:^{
		[gPauseMenuHandler exitGame];
	}];
	viewController.view.frame = metalView.bounds;
	viewController.view.autoresizingMask = UIViewAutoresizingFlexibleWidth | UIViewAutoresizingFlexibleHeight;
	viewController.view.backgroundColor = UIColor.clearColor;

	return viewController;
}

void initMenus(ZGWindow *windowRef, GameState *gameState, void (*exitGame)(ZGWindow *))
{
	gMainMenuHandler = [[MainMenuHandler alloc] init];
	gMainMenuHandler.window = windowRef;
	
	gPauseMenuHandler = [[PauseMenuHandler alloc] init];
	gPauseMenuHandler.window = windowRef;
	gPauseMenuHandler.gameState = gameState;
	gPauseMenuHandler.exitGameFunc = exitGame;
	
	UIView *metalView = metalViewForWindow(windowRef);
	
	gMainMenuController = makeMainMenuController(metalView, gameState);
	gPauseMenuController = makePauseMenuController(metalView);
	
	gCurrentMenuView = gMainMenuController.view;
}

void showGameMenus(ZGWindow *windowRef)
{
	UIView *metalView = metalViewForWindow(windowRef);
	((CAMetalLayer *)metalView.layer).presentsWithTransaction = YES;

	[metalView addSubview:gCurrentMenuView];
#if PLATFORM_TVOS
	[gCurrentMenuView setNeedsFocusUpdate];
#endif
}

void hideGameMenus(ZGWindow *windowRef)
{
	UIView *metalView = metalViewForWindow(windowRef);
	((CAMetalLayer *)metalView.layer).presentsWithTransaction = NO;
	
	[gCurrentMenuView removeFromSuperview];
}

void showPauseMenu(ZGWindow *window, GameState *gameState)
{
#if PLATFORM_TVOS
	ZGUninstallMenuGesture(window);
#else
	ZGUninstallTouchGestures(window);
#endif
	
	gPauseMenuHandler->_resumedGameState = pauseGame(gameState);
	
	UIView *metalView = metalViewForWindow(window);
	((CAMetalLayer *)metalView.layer).presentsWithTransaction = YES;

	gPauseMenuController.view.frame = metalView.bounds;
	[metalView addSubview:gPauseMenuController.view];
	
#if PLATFORM_TVOS
	[gPauseMenuController.view setNeedsFocusUpdate];
#endif
}

void hidePauseMenu(ZGWindow *window, GameState *gameState)
{
	[gPauseMenuHandler resumeGame];
}

#if PLATFORM_TVOS
void performMenuTapAction(ZGWindow *window, GameState *gameState)
{
	if (*gameState == GAME_STATE_ON || *gameState == GAME_STATE_TUTORIAL)
	{
		showPauseMenu(window, gameState);
	}
}
#endif

void performGamepadMenuAction(GamepadEvent *event, GameState *gameState, ZGWindow *window)
{
	if (event->state != GAMEPAD_STATE_PRESSED)
	{
		return;
	}
	
	switch (event->button)
	{
		case GAMEPAD_BUTTON_A:
		case GAMEPAD_BUTTON_START:
			if (gCurrentMenuView == gMainMenuController.view && *gameState == GAME_STATE_OFF)
			{
				playGame(window, false);
			}
			else if (*gameState == GAME_STATE_PAUSED)
			{
				[gPauseMenuHandler resumeGame];
			}
			break;
		case GAMEPAD_BUTTON_B:
		case GAMEPAD_BUTTON_BACK:
		case GAMEPAD_BUTTON_DPAD_UP:
		case GAMEPAD_BUTTON_DPAD_DOWN:
		case GAMEPAD_BUTTON_DPAD_LEFT:
		case GAMEPAD_BUTTON_DPAD_RIGHT:
		case GAMEPAD_BUTTON_X:
		case GAMEPAD_BUTTON_Y:
		case GAMEPAD_BUTTON_LEFTSHOULDER:
		case GAMEPAD_BUTTON_RIGHTSHOULDER:
		case GAMEPAD_BUTTON_LEFTTRIGGER:
		case GAMEPAD_BUTTON_RIGHTTRIGGER:
		case GAMEPAD_BUTTON_MAX:
			break;
	}
}
