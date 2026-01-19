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

static UIView *gMainMenuView;
static UIView *gPauseMenuView;
static UIView *gOptionsView;
static UIView *gOnlineView;
static UIView *gHostGameMenuView;
static UIView *gJoinGameMenuView;
static UIView *gCurrentMenuView;

static void hideGameMenus(ZGWindow *windowRef);

static UIView *metalViewForWindow(ZGWindow *windowRef)
{
	UIWindow *window = (__bridge UIWindow *)(ZGWindowHandle(windowRef));
	UIView *rootView = window.rootViewController.view;
	UIView *metalView = [[rootView subviews] firstObject];
	return metalView;
}

static UIColor *cellTextColor(void)
{
	return [UIColor colorWithRed:0.7 green:0.7 blue:0.7 alpha:1.0];
}

static UIColor *focusCellTextColor(void)
{
	return [UIColor colorWithRed:0.3 green:0.3 blue:0.3 alpha:1.0];
}

static UIColor *cellBackgroundColor(void)
{
	return [UIColor colorWithRed:0.33 green:0.33 blue:0.33 alpha:1.0];
}

#if PLATFORM_TVOS
static NSDictionary *deselectedSegmentedControlTitleAttributes(CGSize metalViewSize)
{
	UIFont *boldTextFont = [UIFont boldSystemFontOfSize:0.0288 * metalViewSize.height];
	UIColor *selectedColor = [UIColor colorWithRed:0.9 green:0.9 blue:0.9 alpha:1.0];
	return @{NSFontAttributeName : boldTextFont, NSForegroundColorAttributeName : selectedColor};
}
#endif

static void setSectionHeaderFont(ZGWindow *window, UIView *view)
{
	UIView *metalView = metalViewForWindow(window);
	if ([view isKindOfClass:[UITableViewHeaderFooterView class]])
	{
		UITableViewHeaderFooterView *header = (UITableViewHeaderFooterView *)view;
		header.textLabel.font = [UIFont boldSystemFontOfSize:metalView.frame.size.height * 0.03];
		header.textLabel.textColor = [UIColor colorWithRed:0.5 green:0.5 blue:0.5 alpha:1.0];
	}
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

- (void)showOnlineMenu
{
	UIView *metalView = metalViewForWindow(_window);
	
	[gCurrentMenuView removeFromSuperview];
	gCurrentMenuView = gOnlineView;
	[metalView addSubview:gCurrentMenuView];
	
#if PLATFORM_TVOS
	ZGInstallMenuGesture(_window);
#endif
}

- (void)showOptions
{
	UIView *metalView = metalViewForWindow(_window);
	
	[gCurrentMenuView removeFromSuperview];
	gCurrentMenuView = gOptionsView;
	[metalView addSubview:gCurrentMenuView];
	
#if PLATFORM_TVOS
	ZGInstallMenuGesture(_window);
#endif
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
	[gPauseMenuView removeFromSuperview];
	
	UIView *metalView = metalViewForWindow(_window);
	((CAMetalLayer *)metalView.layer).presentsWithTransaction = NO;
	
	resumeGame(_resumedGameState, _gameState);
}

- (void)exitGame
{
	UIView *metalView = metalViewForWindow(_window);
	
	[gPauseMenuView removeFromSuperview];
	[metalView addSubview:gCurrentMenuView];
	
	unPauseMusic();
	
	*_gameState = _resumedGameState;
	
	if (_exitGameFunc != NULL)
	{
		_exitGameFunc(_window);
	}
}

@end

@interface OptionsMenuHandler : NSObject <UITableViewDataSource, UITableViewDelegate>

@property (nonatomic) ZGWindow *window;
@property (nonatomic) UITableView *optionsTableView;

@end

@implementation OptionsMenuHandler

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

#if PLATFORM_TVOS
static uint8_t segmentedNumberOfPlayersIndex(void)
{
	uint8_t numberPlayers = numberOfHumanPlayers();
	switch (numberPlayers)
	{
		case 0:
			return 0;
		default:
			return numberPlayers - 1;
	}
}

static uint8_t segmentedNumberOfLivesIndex(void)
{
	int numberOfLives = gCharacterLives;
	switch (numberOfLives)
	{
		case 3:
			return 0;
		case 7:
			return 2;
		default:
			return 1;
	}
}
#endif

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

- (void)_changeNumberOfHumans:(uint8_t)humanPlayers
{
	gPinkBubbleGum.state = CHARACTER_AI_STATE;
	gRedRover.state = CHARACTER_AI_STATE;
	gGreenTree.state = CHARACTER_AI_STATE;
	gBlueLightning.state = CHARACTER_AI_STATE;

	if (humanPlayers > 0)
	{
		gPinkBubbleGum.state = CHARACTER_HUMAN_STATE;

		if (humanPlayers > 1)
		{
			gRedRover.state = CHARACTER_HUMAN_STATE;

			if (humanPlayers > 2)
			{
				gGreenTree.state = CHARACTER_HUMAN_STATE;

				if (humanPlayers > 3)
				{
					gBlueLightning.state = CHARACTER_HUMAN_STATE;
				}
			}
		}
	}

	[_optionsTableView reloadData];
}

#if !PLATFORM_TVOS
- (void)changeNumberOfHumans:(UIStepper *)stepper
{
	[self _changeNumberOfHumans:(uint8_t)stepper.value];
}
#endif

#if PLATFORM_TVOS
- (void)incrementAIMode
{
	if (gAIMode == AI_MEDIUM_MODE)
	{
		gAIMode = AI_HARD_MODE;
	}
	else if (gAIMode == AI_EASY_MODE)
	{
		gAIMode = AI_MEDIUM_MODE;
	}
	else
	{
		gAIMode = AI_EASY_MODE;
	}
	
	[_optionsTableView reloadData];
}

- (void)incrementNumberOfPlayers
{
	uint8_t numberPlayers = numberOfHumanPlayers();
	switch (numberPlayers)
	{
		case 0:
		case 1:
		case 2:
		case 3:
			[self _changeNumberOfHumans:numberPlayers + 1];
			break;
		default:
			[self _changeNumberOfHumans:1];
		
	}
}

- (void)incrementNumberOfLives
{
	switch (gCharacterLives)
	{
		case 5:
			gCharacterLives = 7;
			break;
		case 7:
			gCharacterLives = 3;
			break;
		default:
			gCharacterLives = 5;
	}
	
	[_optionsTableView reloadData];
}

#endif

- (void)changeAIMode:(UISegmentedControl *)segmentedControl
{
	NSInteger segmentIndex = segmentedControl.selectedSegmentIndex;
	if (segmentIndex == 0)
	{
		gAIMode = AI_EASY_MODE;
	}
	else if (segmentIndex == 1)
	{
		gAIMode = AI_MEDIUM_MODE;
	}
	else if (segmentIndex == 2)
	{
		gAIMode = AI_HARD_MODE;
	}
	else
	{
		gAIMode = AI_EASY_MODE;
	}
	
	[_optionsTableView reloadData];
}

- (void)_changeAudioEffectsFlag:(bool)value
{
	gAudioEffectsFlag = value;
	[_optionsTableView reloadData];
}

- (void)_changeAudioMusicFlag:(bool)value
{
	updateAudioMusic(_window, value);

	[_optionsTableView reloadData];
}

#if !PLATFORM_TVOS
- (void)changeNumberOfLives:(UIStepper *)stepper
{
	gCharacterLives = (int)stepper.value;

	[_optionsTableView reloadData];
}

- (void)changeAudioEffectsFlag:(UISwitch *)switchControl
{
	[self _changeAudioEffectsFlag:switchControl.on];
}

- (void)changeAudioMusicFlag:(UISwitch *)switchControl
{
	[self _changeAudioMusicFlag:switchControl.on];
}
#endif

- (nonnull UITableViewCell *)tableView:(nonnull UITableView *)tableView cellForRowAtIndexPath:(nonnull NSIndexPath *)indexPath
{
	UIView *metalView = metalViewForWindow(_window);
	CGSize metalViewSize = metalView.bounds.size;
	
#if PLATFORM_TVOS
	UITableViewCellStyle style = (indexPath.section == 1) ? UITableViewCellStyleValue1 : UITableViewCellStyleDefault;
#else
	UITableViewCellStyle style = UITableViewCellStyleDefault;
#endif
	UITableViewCell *viewCell = [[UITableViewCell alloc] initWithStyle:style reuseIdentifier:nil];
	UIFont *textFont = [UIFont systemFontOfSize:0.0288 * metalViewSize.height];
	viewCell.textLabel.font = textFont;
	viewCell.textLabel.textColor = cellTextColor();
	viewCell.backgroundColor = cellBackgroundColor();
	
	if (indexPath.section == 0 && (indexPath.row == 0 || indexPath.row == 2))
	{
#if PLATFORM_TVOS
		NSArray<NSString *> *items = nil;
		if (indexPath.row == 0)
		{
			// Number of players
			items = @[@"1", @"2", @"3", @"4"];
		}
		else if (indexPath.row == 2)
		{
			// Number of lives
			items = @[@"3", @"5", @"7"];
		}
		
		UISegmentedControl *segmentedControl = [[UISegmentedControl alloc] initWithItems:items];
		NSDictionary *titleAttributes = @{NSFontAttributeName : textFont, NSForegroundColorAttributeName : cellTextColor()};
		[segmentedControl setTitleTextAttributes:titleAttributes forState:UIControlStateNormal];
		
		[segmentedControl setSelectedSegmentTintColor:[UIColor colorWithRed:0.7 green:0.7 blue:0.7 alpha:0.3]];
		
		[segmentedControl setTitleTextAttributes:deselectedSegmentedControlTitleAttributes(metalViewSize) forState:UIControlStateSelected];
		
		if (indexPath.row == 0)
		{
			viewCell.textLabel.text = @"Human Players";
			segmentedControl.selectedSegmentIndex = segmentedNumberOfPlayersIndex();
		}
		else if (indexPath.row == 2)
		{
			viewCell.textLabel.text = @"Player Lives";
			segmentedControl.selectedSegmentIndex = segmentedNumberOfLivesIndex();
		}
		
		viewCell.accessoryView = segmentedControl;
#else
		UIStepper *stepper = [[UIStepper alloc] init];
		// Hack to get tint color respected
		[stepper setDecrementImage:[stepper decrementImageForState:UIControlStateNormal] forState:UIControlStateNormal];
		[stepper setIncrementImage:[stepper incrementImageForState:UIControlStateNormal] forState:UIControlStateNormal];

		stepper.tintColor = cellTextColor();
		stepper.layer.borderColor = UIColor.blackColor.CGColor;
		stepper.wraps = YES;
		stepper.stepValue = 1.0;
		viewCell.accessoryView = stepper;

		if (indexPath.row == 0)
		{
			uint8_t humanPlayers = numberOfHumanPlayers();
			viewCell.textLabel.text = humanPlayers == 1 ? @"1 Human Player" : [NSString stringWithFormat:@"%u Human Players", humanPlayers];
			stepper.minimumValue = 0.0;
			stepper.maximumValue = 4.0;
			stepper.value = (double)humanPlayers;

			[stepper addTarget:self action:@selector(changeNumberOfHumans:) forControlEvents:UIControlEventPrimaryActionTriggered];
		}
		else if (indexPath.row == 2)
		{
			uint8_t numberOfLives = (uint8_t)gCharacterLives;
			viewCell.textLabel.text = numberOfLives == 1 ? @"1 Player Life" : [NSString stringWithFormat:@"%u Player Lives", numberOfLives];
			stepper.minimumValue = 1.0;
			stepper.maximumValue = 10.0;
			stepper.value = (double)numberOfLives;

			[stepper addTarget:self action:@selector(changeNumberOfLives:) forControlEvents:UIControlEventPrimaryActionTriggered];
		}
#endif
	}
	else if (indexPath.section == 0 && indexPath.row == 1)
	{
		UISegmentedControl *segmentedControl = [[UISegmentedControl alloc] initWithItems:@[@"Easy", @"Medium", @"Hard"]];
		NSDictionary *titleAttributes = @{NSFontAttributeName : textFont, NSForegroundColorAttributeName : cellTextColor()};
		[segmentedControl setTitleTextAttributes:titleAttributes forState:UIControlStateNormal];
		
		[segmentedControl setSelectedSegmentTintColor:[UIColor colorWithRed:0.7 green:0.7 blue:0.7 alpha:0.3]];
		
#if PLATFORM_TVOS
		[segmentedControl setTitleTextAttributes:deselectedSegmentedControlTitleAttributes(metalViewSize) forState:UIControlStateSelected];
#endif
		
		segmentedControl.selectedSegmentIndex = currentAIModeIndex();
		
#if !PLATFORM_TVOS
		[segmentedControl addTarget:self action:@selector(changeAIMode:) forControlEvents:UIControlEventValueChanged];
#endif
		
		viewCell.accessoryView = segmentedControl;
		viewCell.textLabel.text = @"Bot Difficulty";
	}
	else if (indexPath.section == 1)
	{
#if PLATFORM_TVOS
		viewCell.detailTextLabel.textColor = cellTextColor();
		viewCell.detailTextLabel.font = textFont;
		
		if (indexPath.row == 0)
		{
			viewCell.textLabel.text = @"Effects";
			viewCell.detailTextLabel.text = gAudioEffectsFlag ? @"On" : @"Off";
		}
		else if (indexPath.row == 1)
		{
			viewCell.textLabel.text = @"Music";
			viewCell.detailTextLabel.text = gAudioMusicFlag ? @"On" : @"Off";
		}
#else
		UISwitch *switchControl = [[UISwitch alloc] initWithFrame:CGRectZero];
		viewCell.accessoryView = switchControl;

		if (indexPath.row == 0)
		{
			viewCell.textLabel.text = @"Effects";
			switchControl.on = (gAudioEffectsFlag != 0);

			[switchControl addTarget:self action:@selector(changeAudioEffectsFlag:) forControlEvents:UIControlEventValueChanged];
		}
		else if (indexPath.row == 1)
		{
			viewCell.textLabel.text = @"Music";
			switchControl.on = (gAudioMusicFlag != 0);

			[switchControl addTarget:self action:@selector(changeAudioMusicFlag:) forControlEvents:UIControlEventValueChanged];
		}
#endif
	}
	
	return viewCell;
}

#if PLATFORM_TVOS
- (void)tableView:(UITableView *)tableView didSelectRowAtIndexPath:(NSIndexPath *)indexPath
{
	if (indexPath.section == 0 && (indexPath.row == 0 || indexPath.row == 2))
	{
		if (indexPath.row == 0)
		{
			[self incrementNumberOfPlayers];
		}
		else
		{
			[self incrementNumberOfLives];
		}
	}
	else if (indexPath.section == 0 && indexPath.row == 1)
	{
		[self incrementAIMode];
	}
	else if (indexPath.section == 1)
	{
		if (indexPath.row == 0)
		{
			[self _changeAudioEffectsFlag:!gAudioEffectsFlag];
		}
		else if (indexPath.row == 1)
		{
			[self _changeAudioMusicFlag:!gAudioMusicFlag];
		}
	}
	
	[tableView deselectRowAtIndexPath:indexPath animated:NO];
}

- (void)tableView:(UITableView *)tableView didUpdateFocusInContext:(UITableViewFocusUpdateContext *)context withAnimationCoordinator:(UIFocusAnimationCoordinator *)coordinator
{
	UIView *metalView = metalViewForWindow(_window);
	CGSize metalViewSize = metalView.bounds.size;
	
	{
		NSIndexPath *nextIndexPath = [context nextFocusedIndexPath];
		if (nextIndexPath != nil)
		{
			UITableViewCell *viewCell = [tableView cellForRowAtIndexPath:nextIndexPath];
			if (viewCell != nil)
			{
				if (nextIndexPath.section == 0)
				{
					UISegmentedControl *segmentedControl = (UISegmentedControl *)viewCell.accessoryView;
					
					UIFont *boldTextFont = [UIFont boldSystemFontOfSize:0.0288 * metalViewSize.height];
					UIColor *selectedColor = [UIColor colorWithRed:0.5 green:0.5 blue:0.5 alpha:1.0];
					NSDictionary *selectedTitleAttributes = @{NSFontAttributeName : boldTextFont, NSForegroundColorAttributeName : selectedColor};
					[segmentedControl setTitleTextAttributes:selectedTitleAttributes forState:UIControlStateSelected];
				}
				else if (nextIndexPath.section == 1)
				{
					viewCell.detailTextLabel.textColor = focusCellTextColor();
				}
				
				viewCell.textLabel.textColor = focusCellTextColor();
			}
		}
	}
	
	{
		NSIndexPath *previousIndexPath = [context previouslyFocusedIndexPath];
		if (previousIndexPath != nil)
		{
			UITableViewCell *viewCell = [tableView cellForRowAtIndexPath:previousIndexPath];
			if (viewCell != nil)
			{
				if (previousIndexPath.section == 0)
				{
					UISegmentedControl *segmentedControl = (UISegmentedControl *)viewCell.accessoryView;
					[segmentedControl setTitleTextAttributes:deselectedSegmentedControlTitleAttributes(metalViewSize) forState:UIControlStateSelected];
				}
				else if (previousIndexPath.section == 1)
				{
					viewCell.detailTextLabel.textColor = cellTextColor();
				}
				
				viewCell.textLabel.textColor = cellTextColor();
			}
		}
	}
}

#endif

- (NSString *)tableView:(UITableView *)tableView titleForHeaderInSection:(NSInteger)section
{
	if (section == 0)
	{
		return @"Gameplay";
	}
	else if (section == 1)
	{
		return @"Audio";
	}
	return nil;
}

- (NSInteger)numberOfSectionsInTableView:(UITableView *)tableView
{
	return 2;
}

- (NSInteger)tableView:(nonnull UITableView *)tableView numberOfRowsInSection:(NSInteger)section
{
	if (section == 0)
	{
		return 3;
	}
	else if (section == 1)
	{
		return 2;
	}
	else
	{
		return 0;
	}
}

- (void)tableView:(UITableView *)tableView willDisplayHeaderView:(UIView *)view forSection:(NSInteger)section
{
	setSectionHeaderFont(_window, view);
}

- (void)navigateBack
{
	UIView *metalView = metalViewForWindow(_window);
	
	[gCurrentMenuView removeFromSuperview];
	gCurrentMenuView = gMainMenuView;
	[metalView addSubview:gCurrentMenuView];
	
#if PLATFORM_TVOS
	ZGUninstallMenuGesture(_window);
#endif
}

@end

@interface HostGameMenuHandler : NSObject <UITableViewDataSource, UITableViewDelegate>

@property (nonatomic) ZGWindow *window;
@property (nonatomic) UITableView *tableView;

@end

@implementation HostGameMenuHandler

- (void)_changeNumberOfNetHumans:(int)newNumberOfNetHumans
{
	gNumberOfNetHumans = newNumberOfNetHumans;
	[_tableView reloadData];
}

#if !PLATFORM_TVOS
- (void)changeNumberOfNetHumans:(UIStepper *)stepper
{
	[self _changeNumberOfNetHumans:(int)stepper.value];
}
#endif

#if PLATFORM_TVOS
static uint8_t segmentedNumberOfNetNumansIndex(void)
{
	switch (gNumberOfNetHumans)
	{
		case 2:
			return 1;
		case 3:
			return 2;
		default:
			return 0;
	}
}

- (void)incrementNumberOfNetHumans
{
	switch (gNumberOfNetHumans)
	{
		case 1:
		case 2:
			[self _changeNumberOfNetHumans:gNumberOfNetHumans + 1];
			break;
		default:
			[self _changeNumberOfNetHumans:1];
	}
}

#endif

- (void)tableView:(UITableView *)tableView didSelectRowAtIndexPath:(NSIndexPath *)indexPath
{
#if PLATFORM_TVOS
	if (indexPath.section == 0)
	{
		[self incrementNumberOfNetHumans];
	}
	else
#endif
	if (indexPath.section == 1)
	{
		if (startNetworkGame(_window))
		{
			hideGameMenus(_window);
			gCurrentMenuView = gMainMenuView;
		}
	}
	
#if PLATFORM_TVOS
	[tableView deselectRowAtIndexPath:indexPath animated:NO];
#endif
}

- (nonnull UITableViewCell *)tableView:(nonnull UITableView *)tableView cellForRowAtIndexPath:(nonnull NSIndexPath *)indexPath
{
	UIView *metalView = metalViewForWindow(_window);
	CGSize metalViewSize = metalView.bounds.size;
	
	UITableViewCell *viewCell = [[UITableViewCell alloc] initWithStyle:UITableViewCellStyleDefault reuseIdentifier:nil];
	UIFont *textFont = [UIFont systemFontOfSize:0.0288 * metalViewSize.height];
	viewCell.textLabel.font = textFont;
	viewCell.textLabel.textColor = cellTextColor();
	viewCell.backgroundColor = cellBackgroundColor();
	viewCell.layer.masksToBounds = YES;
	viewCell.layer.cornerRadius = 8.0;
	viewCell.selectionStyle = UITableViewCellSelectionStyleNone;

	if (indexPath.section == 0)
	{
#if PLATFORM_TVOS
		UISegmentedControl *segmentedControl = [[UISegmentedControl alloc] initWithItems:@[@"1", @"2", @"3"]];
		NSDictionary *titleAttributes = @{NSFontAttributeName : textFont, NSForegroundColorAttributeName : cellTextColor()};
		[segmentedControl setTitleTextAttributes:titleAttributes forState:UIControlStateNormal];
		
		[segmentedControl setSelectedSegmentTintColor:[UIColor colorWithRed:0.7 green:0.7 blue:0.7 alpha:0.3]];
		
		[segmentedControl setTitleTextAttributes:deselectedSegmentedControlTitleAttributes(metalViewSize) forState:UIControlStateSelected];
		
		segmentedControl.selectedSegmentIndex = segmentedNumberOfNetNumansIndex();
		
		viewCell.textLabel.text = @"Friends Joining";
		viewCell.accessoryView = segmentedControl;
#else
		UIStepper *stepper = [[UIStepper alloc] init];
		// Hack to get tint color respected
		[stepper setDecrementImage:[stepper decrementImageForState:UIControlStateNormal] forState:UIControlStateNormal];
		[stepper setIncrementImage:[stepper incrementImageForState:UIControlStateNormal] forState:UIControlStateNormal];

		stepper.tintColor = cellTextColor();
		stepper.layer.borderColor = UIColor.blackColor.CGColor;
		stepper.wraps = YES;
		stepper.stepValue = 1.0;

		stepper.minimumValue = 1.0;
		stepper.maximumValue = 3.0;
		stepper.value = (double)gNumberOfNetHumans;

		[stepper addTarget:self action:@selector(changeNumberOfNetHumans:) forControlEvents:UIControlEventPrimaryActionTriggered];

		viewCell.textLabel.text = gNumberOfNetHumans == 1 ? @"1 Friend Joining" : [NSString stringWithFormat:@"%d Friends Joining", gNumberOfNetHumans];

		viewCell.accessoryView = stepper;
#endif
	}
	else if (indexPath.section == 1)
	{
		viewCell.textLabel.text = @"Start Game";
	}
	
	return viewCell;
}

- (NSInteger)numberOfSectionsInTableView:(UITableView *)tableView
{
	return 2;
}

- (NSInteger)tableView:(nonnull UITableView *)tableView numberOfRowsInSection:(NSInteger)section
{
	return 1;
}

#if PLATFORM_TVOS
- (void)tableView:(UITableView *)tableView didUpdateFocusInContext:(UITableViewFocusUpdateContext *)context withAnimationCoordinator:(UIFocusAnimationCoordinator *)coordinator
{
	UIView *metalView = metalViewForWindow(_window);
	CGSize metalViewSize = metalView.bounds.size;
	
	{
		NSIndexPath *nextIndexPath = [context nextFocusedIndexPath];
		if (nextIndexPath != nil)
		{
			UITableViewCell *viewCell = [tableView cellForRowAtIndexPath:nextIndexPath];
			if (viewCell != nil)
			{
				if (nextIndexPath.section == 0)
				{
					UISegmentedControl *segmentedControl = (UISegmentedControl *)viewCell.accessoryView;
					
					UIFont *boldTextFont = [UIFont boldSystemFontOfSize:0.0288 * metalViewSize.height];
					UIColor *selectedColor = [UIColor colorWithRed:0.5 green:0.5 blue:0.5 alpha:1.0];
					NSDictionary *selectedTitleAttributes = @{NSFontAttributeName : boldTextFont, NSForegroundColorAttributeName : selectedColor};
					[segmentedControl setTitleTextAttributes:selectedTitleAttributes forState:UIControlStateSelected];
				}
				viewCell.textLabel.textColor = focusCellTextColor();
			}
		}
	}
	
	{
		NSIndexPath *previousIndexPath = [context previouslyFocusedIndexPath];
		if (previousIndexPath != nil)
		{
			UITableViewCell *viewCell = [tableView cellForRowAtIndexPath:previousIndexPath];
			if (viewCell != nil)
			{
				if (previousIndexPath.section == 0)
				{
					UISegmentedControl *segmentedControl = (UISegmentedControl *)viewCell.accessoryView;
					[segmentedControl setTitleTextAttributes:deselectedSegmentedControlTitleAttributes(metalViewSize) forState:UIControlStateSelected];
				}
				viewCell.textLabel.textColor = cellTextColor();
			}
		}
	}
}
#endif

- (void)navigateBack
{
	UIView *metalView = metalViewForWindow(_window);
	
	[gCurrentMenuView removeFromSuperview];
	gCurrentMenuView = gOnlineView;
	[metalView addSubview:gCurrentMenuView];
}

@end

//https://stackoverflow.com/questions/2523501/set-uitextfield-maximum-length
static BOOL validateTextFieldChange(UITextField *textField, NSRange range, NSString *string, NSUInteger maxSize, BOOL allowsDash)
{
	NSUInteger oldLength = [textField.text length];
    NSUInteger replacementLength = [string length];
    NSUInteger rangeLength = range.length;
	
    NSUInteger newLength = oldLength - rangeLength + replacementLength;
	
    if (newLength > maxSize - 1)
	{
		return NO;
	}
	
	const char *utf8String = [string UTF8String];
	const char *character = utf8String;
	while (*character != '\0')
	{
		if (!ALLOWED_BASIC_TEXT_INPUT(*character) && (!allowsDash || *character != '-'))
		{
			return NO;
		}
		else
		{
			character++;
		}
	}
	
	return YES;
}

@interface JoinGameMenuHandler : NSObject <UITableViewDataSource, UITableViewDelegate, UITextFieldDelegate>

@property (nonatomic) ZGWindow *window;
@property (nonatomic) GameState *gameState;
@property (nonatomic) UITableView *tableView;
@property (nonatomic) UIButton *cancelButton;

@end

@implementation JoinGameMenuHandler
{
	UITextField *_hostAddresstextField;
}

#if !PLATFORM_TVOS
- (instancetype)init
{
	self = [super init];
	if (self != nil)
	{
		[[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(keyboardWillShow:) name:UIKeyboardWillShowNotification object:nil];

		[[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(keyboardWillDismiss:) name:UIKeyboardWillHideNotification object:nil];
	}
	return self;
}

- (void)keyboardWillShow:(NSNotification *)notification
{
	_cancelButton.hidden = YES;
}

- (void)keyboardWillDismiss:(NSNotification *)notification
{
	_cancelButton.hidden = NO;
}
#endif

- (BOOL)textFieldShouldReturn:(UITextField *)textField
{
	[textField resignFirstResponder];
	
	const char *text = textField.text.UTF8String;
	if (text != NULL)
	{
		strncpy(gServerAddressString, text, MAX_SERVER_ADDRESS_SIZE - 1);
		gServerAddressStringIndex = (int)strlen(gServerAddressString);
	}
	
	[_tableView reloadData];
	
	return YES;
}

- (BOOL)textField:(UITextField *)textField shouldChangeCharactersInRange:(NSRange)range replacementString:(NSString *)string
{
	return validateTextFieldChange(textField, range, string, MAX_SERVER_ADDRESS_SIZE, YES);
}

- (void)tableView:(UITableView *)tableView didSelectRowAtIndexPath:(NSIndexPath *)indexPath
{
	if (indexPath.section == 0)
	{
		[_hostAddresstextField becomeFirstResponder];
	}
	else if (indexPath.section == 1)
	{
		UITableViewCell *viewCell = [tableView cellForRowAtIndexPath:indexPath];
		if (viewCell.textLabel.enabled && connectToNetworkGame(_gameState))
		{
			hideGameMenus(_window);
		}
	}
}

- (nonnull UITableViewCell *)tableView:(nonnull UITableView *)tableView cellForRowAtIndexPath:(nonnull NSIndexPath *)indexPath
{
	UIView *metalView = metalViewForWindow(_window);
	CGSize metalViewSize = metalView.bounds.size;
	
	UITableViewCell *viewCell = [[UITableViewCell alloc] initWithStyle:UITableViewCellStyleDefault reuseIdentifier:nil];
	UIFont *textFont = [UIFont systemFontOfSize:0.0288 * metalViewSize.height];
	viewCell.textLabel.font = textFont;
	viewCell.textLabel.textColor = cellTextColor();
	viewCell.backgroundColor = cellBackgroundColor();
	viewCell.layer.masksToBounds = YES;
	viewCell.layer.cornerRadius = 8.0;
	viewCell.selectionStyle = UITableViewCellSelectionStyleNone;

	if (indexPath.section == 0)
	{
		viewCell.textLabel.text = @"Host Address";
		
		UITextField *textField = [[UITextField alloc] initWithFrame:CGRectMake(0.0, 0.0, 0.2 * metalViewSize.width, 0.12 * metalViewSize.height)];
		textField.text = [NSString stringWithCString:gServerAddressString encoding:NSUTF8StringEncoding];
		textField.textColor = cellTextColor();
		textField.font = textFont;
		[textField setReturnKeyType:UIReturnKeyDone];
		textField.delegate = self;
		textField.autocorrectionType = UITextAutocorrectionTypeNo;
		textField.autocapitalizationType = UITextAutocapitalizationTypeNone;
		textField.smartDashesType = UITextSmartDashesTypeNo;
		textField.keyboardType = UIKeyboardTypeNumbersAndPunctuation;
		
		_hostAddresstextField = textField;
		viewCell.accessoryView = textField;
	}
	else if (indexPath.section == 1)
	{
		viewCell.textLabel.text = @"Connect";
		viewCell.textLabel.enabled = strlen(gServerAddressString) > 0;
	}
	
	return viewCell;
}

- (NSInteger)numberOfSectionsInTableView:(UITableView *)tableView
{
	return 2;
}

- (NSInteger)tableView:(nonnull UITableView *)tableView numberOfRowsInSection:(NSInteger)section
{
	return 1;
}

#if PLATFORM_TVOS
- (void)tableView:(UITableView *)tableView didUpdateFocusInContext:(UITableViewFocusUpdateContext *)context withAnimationCoordinator:(UIFocusAnimationCoordinator *)coordinator
{
	{
		NSIndexPath *nextIndexPath = [context nextFocusedIndexPath];
		if (nextIndexPath != nil)
		{
			UITableViewCell *viewCell = [tableView cellForRowAtIndexPath:nextIndexPath];
			if (viewCell != nil)
			{
				if (nextIndexPath.section == 0)
				{
					_hostAddresstextField.textColor = focusCellTextColor();
				}
				viewCell.textLabel.textColor = focusCellTextColor();
			}
		}
	}
	
	{
		NSIndexPath *previousIndexPath = [context previouslyFocusedIndexPath];
		if (previousIndexPath != nil)
		{
			UITableViewCell *viewCell = [tableView cellForRowAtIndexPath:previousIndexPath];
			if (viewCell != nil)
			{
				if (previousIndexPath.section == 0)
				{
					_hostAddresstextField.textColor = cellTextColor();
				}
				viewCell.textLabel.textColor = cellTextColor();
			}
		}
	}
}
#endif

- (void)navigateBack
{
	UIView *metalView = metalViewForWindow(_window);
	
	[gCurrentMenuView removeFromSuperview];
	gCurrentMenuView = gOnlineView;
	[metalView addSubview:gCurrentMenuView];
}

@end

@interface OnlineMenuHandler : NSObject <UITableViewDataSource, UITableViewDelegate, UITextFieldDelegate>

@property (nonatomic) ZGWindow *window;
@property (nonatomic) UITableView *tableView;
@property (nonatomic) UIButton *cancelButton;

@end

@implementation OnlineMenuHandler
{
	UITextField *_onlineNameTextField;
}

#if !PLATFORM_TVOS
- (instancetype)init
{
	self = [super init];
	if (self != nil)
	{
		[[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(keyboardWillShow:) name:UIKeyboardWillShowNotification object:nil];

		[[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(keyboardWillDismiss:) name:UIKeyboardWillHideNotification object:nil];
	}
	return self;
}

- (void)keyboardWillShow:(NSNotification *)notification
{
	_cancelButton.hidden = YES;
}

- (void)keyboardWillDismiss:(NSNotification *)notification
{
	_cancelButton.hidden = NO;
}
#endif

- (BOOL)textFieldShouldReturn:(UITextField *)textField
{
	[textField resignFirstResponder];
	
	const char *text = textField.text.UTF8String;
	if (text != NULL)
	{
		strncpy(gUserNameString, text, MAX_USER_NAME_SIZE - 1);
		gUserNameStringIndex = (int)strlen(gUserNameString);
	}
	
	return YES;
}

- (BOOL)textField:(UITextField *)textField shouldChangeCharactersInRange:(NSRange)range replacementString:(NSString *)string
{
	return validateTextFieldChange(textField, range, string, MAX_USER_NAME_SIZE, NO);
}

- (void)tableView:(UITableView *)tableView didSelectRowAtIndexPath:(NSIndexPath *)indexPath
{
	if (indexPath.section == 0)
	{
		[_onlineNameTextField becomeFirstResponder];
	}
	else if (indexPath.section == 1)
	{
		UIView *metalView = metalViewForWindow(_window);
		
		[gCurrentMenuView removeFromSuperview];
		gCurrentMenuView = gHostGameMenuView;
		[metalView addSubview:gCurrentMenuView];
	}
	else if (indexPath.section == 2)
	{
		UIView *metalView = metalViewForWindow(_window);
		
		[gCurrentMenuView removeFromSuperview];
		gCurrentMenuView = gJoinGameMenuView;
		[metalView addSubview:gCurrentMenuView];
	}
}

- (nonnull UITableViewCell *)tableView:(nonnull UITableView *)tableView cellForRowAtIndexPath:(nonnull NSIndexPath *)indexPath
{
	UIView *metalView = metalViewForWindow(_window);
	CGSize metalViewSize = metalView.bounds.size;
	
	UITableViewCell *viewCell = [[UITableViewCell alloc] initWithStyle:UITableViewCellStyleDefault reuseIdentifier:nil];
	UIFont *textFont = [UIFont systemFontOfSize:0.0288 * metalViewSize.height];
	viewCell.textLabel.font = textFont;
	viewCell.textLabel.textColor = cellTextColor();
	viewCell.backgroundColor = cellBackgroundColor();
	viewCell.layer.masksToBounds = YES;
	viewCell.layer.cornerRadius = 8.0;
	viewCell.selectionStyle = UITableViewCellSelectionStyleNone;
	
	if (indexPath.section == 0)
	{
		viewCell.textLabel.text = @"Online Name";
		
		UITextField *textField = [[UITextField alloc] initWithFrame:CGRectMake(0.0, 0.0, 0.093 * metalViewSize.width, 0.12 * metalViewSize.height)];
		textField.text = [NSString stringWithCString:gUserNameString encoding:NSUTF8StringEncoding];
		textField.textColor = cellTextColor();
		textField.font = textFont;
		[textField setReturnKeyType:UIReturnKeyDone];
		textField.delegate = self;
		
		_onlineNameTextField = textField;
		
		viewCell.accessoryView = textField;
	}
	else if (indexPath.section == 1)
	{
		viewCell.textLabel.text = @"Make Game";
	}
	else if (indexPath.section == 2)
	{
		viewCell.textLabel.text = @"Join Game";
	}
	
	return viewCell;
}

- (NSInteger)numberOfSectionsInTableView:(UITableView *)tableView
{
	return 3;
}

- (NSInteger)tableView:(nonnull UITableView *)tableView numberOfRowsInSection:(NSInteger)section
{
	return 1;
}

- (void)tableView:(UITableView *)tableView willDisplayHeaderView:(UIView *)view forSection:(NSInteger)section
{
	setSectionHeaderFont(_window, view);
}

#if PLATFORM_TVOS
- (void)tableView:(UITableView *)tableView didUpdateFocusInContext:(UITableViewFocusUpdateContext *)context withAnimationCoordinator:(UIFocusAnimationCoordinator *)coordinator
{
	{
		NSIndexPath *nextIndexPath = [context nextFocusedIndexPath];
		if (nextIndexPath != nil)
		{
			UITableViewCell *viewCell = [tableView cellForRowAtIndexPath:nextIndexPath];
			if (viewCell != nil)
			{
				if (nextIndexPath.section == 0)
				{
					_onlineNameTextField.textColor = focusCellTextColor();
				}
				viewCell.textLabel.textColor = focusCellTextColor();
			}
		}
	}
	
	{
		NSIndexPath *previousIndexPath = [context previouslyFocusedIndexPath];
		if (previousIndexPath != nil)
		{
			UITableViewCell *viewCell = [tableView cellForRowAtIndexPath:previousIndexPath];
			if (viewCell != nil)
			{
				if (previousIndexPath.section == 0)
				{
					_onlineNameTextField.textColor = cellTextColor();
				}
				viewCell.textLabel.textColor = cellTextColor();
			}
		}
	}
}
#endif

- (void)navigateBack
{
	UIView *metalView = metalViewForWindow(_window);
	
	[gCurrentMenuView removeFromSuperview];
	gCurrentMenuView = gMainMenuView;
	[metalView addSubview:gCurrentMenuView];
	
#if PLATFORM_TVOS
	ZGUninstallMenuGesture(_window);
#endif
}

@end

static MainMenuHandler *gMainMenuHandler;
static PauseMenuHandler *gPauseMenuHandler;
static OnlineMenuHandler *gOnlineMenuHandler;
static HostGameMenuHandler *gHostGameMenuHandler;
static JoinGameMenuHandler *gJoinGameMenuHandler;
static OptionsMenuHandler *gOptionsMenuHandler;

static UIView *makeMainMenu(UIView *metalView)
{
	UIView *menuView = [[UIView alloc] initWithFrame:metalView.frame];
	CGSize metalViewSize = metalView.bounds.size;
	
	CGFloat buttonWidth = 0.15 * metalViewSize.width;
	CGFloat buttonHeight = 0.08 * metalViewSize.height;
	
	CGFloat fontSize = 0.03 * metalViewSize.width;
	UIFont *font = [UIFont systemFontOfSize:fontSize];
	UIColor *titleColor = cellTextColor();
	UIColor *focusTitleColor = focusCellTextColor();
	CGColorRef buttonBackgroundColor = cellBackgroundColor().CGColor;
	CGFloat borderWidth = 0.0;
	CGFloat cornerRadius = 9.0;
	
	menuView.opaque = NO;
	
	{
		UIButton *button = [UIButton buttonWithType:UIButtonTypeRoundedRect];
		[button setTitle:@"Play" forState:UIControlStateNormal];
		
		button.titleLabel.font = font;
		[button setTitleColor:titleColor forState:UIControlStateNormal];
		[button setTitleColor:focusTitleColor forState:UIControlStateFocused];
		
		button.layer.backgroundColor = buttonBackgroundColor;
		button.layer.borderWidth = borderWidth;
		button.layer.cornerRadius = cornerRadius;
		
		button.frame = CGRectMake(metalViewSize.width * 0.5f - buttonWidth / 2.0, metalViewSize.height * 0.4 - buttonHeight / 2.0, buttonWidth, buttonHeight);
		
		[button addTarget:gMainMenuHandler action:@selector(playGame) forControlEvents:UIControlEventPrimaryActionTriggered];
		
		[menuView addSubview:button];
	}
	
	{
		UIButton *button = [UIButton buttonWithType:UIButtonTypeRoundedRect];
		[button setTitle:@"Online" forState:UIControlStateNormal];
		
		button.titleLabel.font = font;
		[button setTitleColor:titleColor forState:UIControlStateNormal];
		[button setTitleColor:focusTitleColor forState:UIControlStateFocused];
		
		button.layer.backgroundColor = buttonBackgroundColor;
		button.layer.borderWidth = borderWidth;
		button.layer.cornerRadius = cornerRadius;
		
		button.frame = CGRectMake(metalViewSize.width * 0.5f - buttonWidth / 2.0, metalViewSize.height * 0.55 - buttonHeight / 2.0, buttonWidth, buttonHeight);
		
		[button addTarget:gMainMenuHandler action:@selector(showOnlineMenu) forControlEvents:UIControlEventPrimaryActionTriggered];
		
		[menuView addSubview:button];
	}
	
	{
		UIButton *button = [UIButton buttonWithType:UIButtonTypeRoundedRect];
		[button setTitle:@"Tutorial" forState:UIControlStateNormal];
		
		button.titleLabel.font = font;
		[button setTitleColor:titleColor forState:UIControlStateNormal];
		[button setTitleColor:focusTitleColor forState:UIControlStateFocused];
		
		button.layer.backgroundColor = buttonBackgroundColor;
		button.layer.borderWidth = borderWidth;
		button.layer.cornerRadius = cornerRadius;
		
		button.frame = CGRectMake(metalViewSize.width * 0.5f - buttonWidth / 2.0, metalViewSize.height * 0.7 - buttonHeight / 2.0, buttonWidth, buttonHeight);
		
		[button addTarget:gMainMenuHandler action:@selector(playTutorial) forControlEvents:UIControlEventPrimaryActionTriggered];
		
		[menuView addSubview:button];
	}
	
	{
		UIButton *button = [UIButton buttonWithType:UIButtonTypeRoundedRect];
		[button setTitle:@"Options" forState:UIControlStateNormal];
		
		button.titleLabel.font = font;
		[button setTitleColor:titleColor forState:UIControlStateNormal];
		[button setTitleColor:focusTitleColor forState:UIControlStateFocused];
		
		button.layer.backgroundColor = buttonBackgroundColor;
		button.layer.borderWidth = borderWidth;
		button.layer.cornerRadius = cornerRadius;
		
		button.frame = CGRectMake(metalViewSize.width * 0.5f - buttonWidth / 2.0, metalViewSize.height * 0.85 - buttonHeight / 2.0, buttonWidth, buttonHeight);
		
		[button addTarget:gMainMenuHandler action:@selector(showOptions) forControlEvents:UIControlEventPrimaryActionTriggered];
		
		[menuView addSubview:button];
	}
	
	return menuView;
}

static UIView *makePauseMenu(UIView *metalView)
{
	UIView *menuView = [[UIView alloc] initWithFrame:metalView.frame];
	CGSize metalViewSize = metalView.bounds.size;
	
	CGFloat buttonWidth = 0.15 * metalViewSize.width;
	CGFloat buttonHeight = 0.08 * metalViewSize.height;
	
	CGFloat fontSize = 0.03 * metalViewSize.width;
	UIFont *font = [UIFont systemFontOfSize:fontSize];
	UIColor *titleColor = cellTextColor();
	UIColor *focusTitleColor = focusCellTextColor();
	CGColorRef buttonBackgroundColor = cellBackgroundColor().CGColor;
	CGFloat borderWidth = 0.0;
	CGFloat cornerRadius = 9.0;
	
	menuView.opaque = NO;
	
	{
		UIButton *button = [UIButton buttonWithType:UIButtonTypeRoundedRect];
		[button setTitle:@"Resume" forState:UIControlStateNormal];
		
		button.titleLabel.font = font;
		[button setTitleColor:titleColor forState:UIControlStateNormal];
		[button setTitleColor:focusTitleColor forState:UIControlStateFocused];
		
		button.layer.backgroundColor = buttonBackgroundColor;
		button.layer.borderWidth = borderWidth;
		button.layer.cornerRadius = cornerRadius;
		
		button.frame = CGRectMake(metalViewSize.width * 0.5f - buttonWidth / 2.0, metalViewSize.height * 0.4 - buttonHeight / 2.0, buttonWidth, buttonHeight);
		
		[button addTarget:gPauseMenuHandler action:@selector(resumeGame) forControlEvents:UIControlEventPrimaryActionTriggered];
		
		[menuView addSubview:button];
	}
	
	{
		UIButton *button = [UIButton buttonWithType:UIButtonTypeRoundedRect];
		[button setTitle:@"Exit" forState:UIControlStateNormal];
		
		button.titleLabel.font = font;
		[button setTitleColor:titleColor forState:UIControlStateNormal];
		[button setTitleColor:focusTitleColor forState:UIControlStateFocused];
		
		button.layer.backgroundColor = buttonBackgroundColor;
		button.layer.borderWidth = borderWidth;
		button.layer.cornerRadius = cornerRadius;
		
		button.frame = CGRectMake(metalViewSize.width * 0.5f - buttonWidth / 2.0, metalViewSize.height * 0.55 - buttonHeight / 2.0, buttonWidth, buttonHeight);
		
		[button addTarget:gPauseMenuHandler action:@selector(exitGame) forControlEvents:UIControlEventPrimaryActionTriggered];
		
		[menuView addSubview:button];
	}
	
	return menuView;
}

#if !PLATFORM_TVOS
static UIButton *makeCancelButton(CGSize metalViewSize)
{
	UIButton *cancelButton = [UIButton buttonWithType:UIButtonTypeRoundedRect];
	
	CGFloat buttonSize = metalViewSize.width * 0.0599;
	cancelButton.frame = CGRectMake(metalViewSize.width * 0.12, metalViewSize.height * 0.30, buttonSize, buttonSize);
	cancelButton.titleLabel.font = [UIFont systemFontOfSize:metalViewSize.width * 0.032];
	cancelButton.layer.borderWidth = 2.0;
	cancelButton.layer.borderColor = cellTextColor().CGColor;
	cancelButton.layer.cornerRadius = 9.0;
	[cancelButton setTitle:@"✗" forState:UIControlStateNormal];
	[cancelButton setTitleColor:cellTextColor() forState:UIControlStateNormal];
	
	return cancelButton;
}
#endif

static UITableView *makeTableView(CGSize metalViewSize, CGFloat widthFactor)
{
	CGFloat width = metalViewSize.width * widthFactor;
	CGFloat originY = metalViewSize.height * 0.25;
	CGFloat height = metalViewSize.height - originY;
	
	UITableView *tableView = [[UITableView alloc] initWithFrame:CGRectMake(metalViewSize.width / 2.0 - width / 2.0, originY, width, height) style:UITableViewStyleGrouped];
	tableView.opaque = NO;
	tableView.backgroundColor = UIColor.clearColor;
	tableView.rowHeight = metalViewSize.height * 0.09;
	tableView.sectionFooterHeight = 0.0;
	tableView.sectionHeaderHeight = metalViewSize.height * 0.1;
	tableView.bounces = NO;
	
	return tableView;
}

static UIView *makeOptionsMenu(UIView *metalView)
{
	UIView *optionsMenu = [[UIView alloc] initWithFrame:metalView.frame];
	optionsMenu.opaque = NO;
	
	CGSize metalViewSize = metalView.bounds.size;
	
	UITableView *tableView = makeTableView(metalViewSize, 0.5);
	tableView.dataSource = gOptionsMenuHandler;
	tableView.delegate = gOptionsMenuHandler;
	gOptionsMenuHandler.optionsTableView = tableView;
	
	[optionsMenu addSubview:tableView];
	
#if !PLATFORM_TVOS
	tableView.allowsSelection = NO;
	
	UIButton *cancelButton = makeCancelButton(metalViewSize);
	[cancelButton addTarget:gOptionsMenuHandler action:@selector(navigateBack) forControlEvents:UIControlEventPrimaryActionTriggered];
	[optionsMenu addSubview:cancelButton];
#endif
	
	return optionsMenu;
}

static UIView *makeOnlineMenu(UIView *metalView)
{
	UIView *onlineMenu = [[UIView alloc] initWithFrame:metalView.frame];
	onlineMenu.opaque = NO;
	
	CGSize metalViewSize = metalView.bounds.size;
	
	UITableView *tableView = makeTableView(metalViewSize, 0.4);
	tableView.dataSource = gOnlineMenuHandler;
	tableView.delegate = gOnlineMenuHandler;
#if !PLATFORM_TVOS
	tableView.separatorStyle = UITableViewCellSeparatorStyleNone;
#endif
	gOnlineMenuHandler.tableView = tableView;
	[onlineMenu addSubview:tableView];
	
#if !PLATFORM_TVOS
	UIButton *cancelButton = makeCancelButton(metalViewSize);
	[cancelButton addTarget:gOnlineMenuHandler action:@selector(navigateBack) forControlEvents:UIControlEventPrimaryActionTriggered];
	gOnlineMenuHandler.cancelButton = cancelButton;
	[onlineMenu addSubview:cancelButton];
#endif
	
	return onlineMenu;
}

static UIView *makeHostGameMenu(UIView *metalView)
{
	UIView *view = [[UIView alloc] initWithFrame:metalView.frame];
	view.opaque = NO;
	
	CGSize metalViewSize = metalView.bounds.size;
	
	UITableView *tableView = makeTableView(metalViewSize, 0.45);
#if !PLATFORM_TVOS
	tableView.separatorStyle = UITableViewCellSeparatorStyleNone;
#endif
	tableView.dataSource = gHostGameMenuHandler;
	tableView.delegate = gHostGameMenuHandler;
	gHostGameMenuHandler.tableView = tableView;
	[view addSubview:tableView];
	
#if !PLATFORM_TVOS
	UIButton *cancelButton = makeCancelButton(metalViewSize);
	[cancelButton addTarget:gHostGameMenuHandler action:@selector(navigateBack) forControlEvents:UIControlEventPrimaryActionTriggered];
	[view addSubview:cancelButton];
#endif
	
	return view;
}

static UIView *makeJoinGameMenu(UIView *metalView)
{
	UIView *view = [[UIView alloc] initWithFrame:metalView.frame];
	view.opaque = NO;
	
	CGSize metalViewSize = metalView.bounds.size;
	
	UITableView *tableView = makeTableView(metalViewSize, 0.45);
#if !PLATFORM_TVOS
	tableView.separatorStyle = UITableViewCellSeparatorStyleNone;
#endif
	tableView.dataSource = gJoinGameMenuHandler;
	tableView.delegate = gJoinGameMenuHandler;
	gJoinGameMenuHandler.tableView = tableView;
	[view addSubview:tableView];
	
#if !PLATFORM_TVOS
	UIButton *cancelButton = makeCancelButton(metalViewSize);
	[cancelButton addTarget:gJoinGameMenuHandler action:@selector(navigateBack) forControlEvents:UIControlEventPrimaryActionTriggered];
	gJoinGameMenuHandler.cancelButton = cancelButton;
	[view addSubview:cancelButton];
#endif
	
	return view;
}

void initMenus(ZGWindow *windowRef, GameState *gameState, void (*exitGame)(ZGWindow *))
{
	gMainMenuHandler = [[MainMenuHandler alloc] init];
	gMainMenuHandler.window = windowRef;
	
	gPauseMenuHandler = [[PauseMenuHandler alloc] init];
	gPauseMenuHandler.window = windowRef;
	gPauseMenuHandler.gameState = gameState;
	gPauseMenuHandler.exitGameFunc = exitGame;
	
	gOnlineMenuHandler = [[OnlineMenuHandler alloc] init];
	gOnlineMenuHandler.window = windowRef;
	
	gHostGameMenuHandler = [[HostGameMenuHandler alloc] init];
	gHostGameMenuHandler.window = windowRef;
	
	gJoinGameMenuHandler = [[JoinGameMenuHandler alloc] init];
	gJoinGameMenuHandler.window = windowRef;
	gJoinGameMenuHandler.gameState = gameState;
	
	gOptionsMenuHandler = [[OptionsMenuHandler alloc] init];
	gOptionsMenuHandler.window = windowRef;
	
	UIView *metalView = metalViewForWindow(windowRef);
	
	gMainMenuView = makeMainMenu(metalView);
	gPauseMenuView = makePauseMenu(metalView);
	gOnlineView = makeOnlineMenu(metalView);
	gHostGameMenuView = makeHostGameMenu(metalView);
	gJoinGameMenuView = makeJoinGameMenu(metalView);
	gOptionsView = makeOptionsMenu(metalView);

	gCurrentMenuView = gMainMenuView;
	
#if !PLATFORM_TVOS
	UIView *(^restoreMenuView)(UIView *, UIView *(*)(UIView *)) = ^(UIView *oldTargetMenuView, UIView *(*makeMenuFunc)(UIView *)) {
		BOOL currentMenuWasTargetMenu = (gCurrentMenuView == oldTargetMenuView);
		BOOL targetMenuWasActive = (oldTargetMenuView.superview != nil);
		
		if (targetMenuWasActive)
		{
			[oldTargetMenuView removeFromSuperview];
		}
		
		UIView *newTargetMenuView = makeMenuFunc(metalView);
		
		if (currentMenuWasTargetMenu)
		{
			gCurrentMenuView = newTargetMenuView;
		}
		
		if (targetMenuWasActive)
		{
			[metalView addSubview:newTargetMenuView];
		}
		
		return newTargetMenuView;
	};
	
	// Re-create menu items when viewport changes so they reflect the new size
	[[NSNotificationCenter defaultCenter] addObserverForName:ZGMetalViewportChangedNotification object:nil queue:NSOperationQueue.mainQueue usingBlock:^(NSNotification * _Nonnull __unused notification) {
		gMainMenuView = restoreMenuView(gMainMenuView, makeMainMenu);
		gPauseMenuView = restoreMenuView(gPauseMenuView, makePauseMenu);
		gOnlineView = restoreMenuView(gOnlineView, makeOnlineMenu);
		gHostGameMenuView = restoreMenuView(gHostGameMenuView, makeHostGameMenu);
		gJoinGameMenuView = restoreMenuView(gJoinGameMenuView, makeJoinGameMenu);
		gOptionsView = restoreMenuView(gOptionsView, makeOptionsMenu);
	}];
#endif
}

void showGameMenus(ZGWindow *windowRef)
{
	UIView *metalView = metalViewForWindow(windowRef);
	((CAMetalLayer *)metalView.layer).presentsWithTransaction = YES;
	
	[metalView addSubview:gCurrentMenuView];
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
	
	[metalView addSubview:gPauseMenuView];
	
#if PLATFORM_TVOS
	[gPauseMenuView setNeedsFocusUpdate];
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
	else if (gCurrentMenuView == gOptionsView)
	{
		[gOptionsMenuHandler navigateBack];
	}
	else if (gCurrentMenuView == gOnlineView)
	{
		[gOnlineMenuHandler navigateBack];
	}
	else if (gCurrentMenuView == gHostGameMenuView)
	{
		[gHostGameMenuHandler navigateBack];
	}
	else if (gCurrentMenuView == gJoinGameMenuView)
	{
		[gJoinGameMenuHandler navigateBack];
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
			if (gCurrentMenuView == gMainMenuView && *gameState == GAME_STATE_OFF)
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
