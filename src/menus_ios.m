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

static UIColor *cellBackgroundColor(void)
{
	return [UIColor colorWithRed:0.33 green:0.33 blue:0.33 alpha:1.0];
}

static void setSectionHeaderFont(ZGWindow *window, UIView *view)
{
	UIView *metalView = metalViewForWindow(window);
    UITableViewHeaderFooterView *header = (UITableViewHeaderFooterView *)view;
	header.textLabel.font = [UIFont boldSystemFontOfSize:metalView.frame.size.height * 0.03];
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
	playGame(_window, false);
}

- (void)playTutorial
{
	playGame(_window, true);
}

- (void)showOnlineMenu
{
	UIView *metalView = metalViewForWindow(_window);
	
	[gCurrentMenuView removeFromSuperview];
	gCurrentMenuView = gOnlineView;
	[metalView addSubview:gCurrentMenuView];
}

- (void)showOptions
{
	UIView *metalView = metalViewForWindow(_window);
	
	[gCurrentMenuView removeFromSuperview];
	gCurrentMenuView = gOptionsView;
	[metalView addSubview:gCurrentMenuView];
}

@end

@interface PauseMenuHandler : NSObject

@property (nonatomic) ZGWindow *window;
@property (nonatomic) GameState *gameState;
@property (nonatomic) GameState resumedGameState;
@property (nonatomic) void (*exitGameFunc)(ZGWindow *);

@end

@implementation PauseMenuHandler

- (void)resumeGame
{
	unPauseMusic();
	ZGInstallTouchGestures(_window);
	[gPauseMenuView removeFromSuperview];
	
	UIView *metalView = metalViewForWindow(_window);
	((CAMetalLayer *)metalView.layer).presentsWithTransaction = NO;
	
	*_gameState = _resumedGameState;
}

- (void)exitGame
{
	UIView *metalView = metalViewForWindow(_window);
	
	[gPauseMenuView removeFromSuperview];
	[metalView addSubview:gCurrentMenuView];
	
	unPauseMusic();
	
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

#if !PLATFORM_TVOS
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

#if !PLATFORM_TVOS
- (void)changeNumberOfHumans:(UIStepper *)stepper
{
	uint8_t humanPlayers = (uint8_t)stepper.value;

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

#if !PLATFORM_TVOS
- (void)changeNumberOfLives:(UIStepper *)stepper
{
	gCharacterLives = (int)stepper.value;

	[_optionsTableView reloadData];
}

- (void)changeAudioEffectsFlag:(UISwitch *)switchControl
{
	gAudioEffectsFlag = switchControl.on;

	[_optionsTableView reloadData];
}

- (void)changeAudioMusicFlag:(UISwitch *)switchControl
{
	updateAudioMusic(_window, switchControl.on);

	[_optionsTableView reloadData];
}
#endif

- (nonnull UITableViewCell *)tableView:(nonnull UITableView *)tableView cellForRowAtIndexPath:(nonnull NSIndexPath *)indexPath
{
	UIView *metalView = metalViewForWindow(_window);
	CGSize metalViewSize = metalView.bounds.size;
	
	UITableViewCell *viewCell = [[UITableViewCell alloc] initWithStyle:UITableViewCellStyleDefault reuseIdentifier:nil];
	UIFont *textFont = [UIFont systemFontOfSize:0.0288 * metalViewSize.height];
	viewCell.textLabel.font = textFont;
	viewCell.textLabel.textColor = cellTextColor();
	viewCell.backgroundColor = cellBackgroundColor();
	
	if (indexPath.section == 0 && (indexPath.row == 0 || indexPath.row == 2))
	{
#if !PLATFORM_TVOS
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
		[segmentedControl setTitleTextAttributes:titleAttributes forState:UIControlStateSelected];
		[segmentedControl setSelectedSegmentTintColor:[UIColor colorWithRed:0.7 green:0.7 blue:0.7 alpha:0.3]];
		
		segmentedControl.selectedSegmentIndex = currentAIModeIndex();
		
		[segmentedControl addTarget:self action:@selector(changeAIMode:) forControlEvents:UIControlEventValueChanged];
		viewCell.accessoryView = segmentedControl;
		
		viewCell.textLabel.text = @"Bot Difficulty";
	}
	else if (indexPath.section == 1)
	{
#if !PLATFORM_TVOS
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
}

@end

@interface HostGameMenuHandler : NSObject <UITableViewDataSource, UITableViewDelegate>

@property (nonatomic) ZGWindow *window;
@property (nonatomic) UITableView *tableView;

@end

@implementation HostGameMenuHandler

- (void)tableView:(UITableView *)tableView didSelectRowAtIndexPath:(NSIndexPath *)indexPath
{
	if (indexPath.section == 1)
	{
		if (startNetworkGame(_window))
		{
			hideGameMenus(_window);
			gCurrentMenuView = gMainMenuView;
		}
	}
}

#if !PLATFORM_TVOS
- (void)changeNumberOfNetHumans:(UIStepper *)stepper
{
	gNumberOfNetHumans = (int)stepper.value;
	[_tableView reloadData];
}
#endif

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
#if !PLATFORM_TVOS
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

- (void)navigateBack
{
	UIView *metalView = metalViewForWindow(_window);
	
	[gCurrentMenuView removeFromSuperview];
	gCurrentMenuView = gOnlineView;
	[metalView addSubview:gCurrentMenuView];
}

@end

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

//https://stackoverflow.com/questions/2523501/set-uitextfield-maximum-length
- (BOOL)textField:(UITextField *) textField shouldChangeCharactersInRange:(NSRange)range replacementString:(NSString *)string
{
    NSUInteger oldLength = [textField.text length];
    NSUInteger replacementLength = [string length];
    NSUInteger rangeLength = range.length;
	
    NSUInteger newLength = oldLength - rangeLength + replacementLength;
	
    return newLength <= MAX_SERVER_ADDRESS_SIZE - 1;
}

- (void)tableView:(UITableView *)tableView didSelectRowAtIndexPath:(NSIndexPath *)indexPath
{
	if (indexPath.section == 1)
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
		textField.placeholder = @"localhost";
		textField.autocorrectionType = UITextAutocorrectionTypeNo;
		textField.autocapitalizationType = UITextAutocapitalizationTypeNone;
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

//https://stackoverflow.com/questions/2523501/set-uitextfield-maximum-length
- (BOOL)textField:(UITextField *) textField shouldChangeCharactersInRange:(NSRange)range replacementString:(NSString *)string
{
    NSUInteger oldLength = [textField.text length];
    NSUInteger replacementLength = [string length];
    NSUInteger rangeLength = range.length;
	
    NSUInteger newLength = oldLength - rangeLength + replacementLength;
	
    return newLength <= MAX_USER_NAME_SIZE - 1;
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
		textField.placeholder = @"Blob";
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

- (void)navigateBack
{
	UIView *metalView = metalViewForWindow(_window);
	
	[gCurrentMenuView removeFromSuperview];
	gCurrentMenuView = gMainMenuView;
	[metalView addSubview:gCurrentMenuView];
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
	CGColorRef buttonBackgroundColor = cellBackgroundColor().CGColor;
	CGFloat borderWidth = 0.0;
	CGFloat cornerRadius = 9.0;
	
	menuView.opaque = NO;
	
	{
		UIButton *button = [UIButton buttonWithType:UIButtonTypeRoundedRect];
		[button setTitle:@"Play" forState:UIControlStateNormal];
		
		button.titleLabel.font = font;
		[button setTitleColor:titleColor forState:UIControlStateNormal];
		
		button.layer.backgroundColor = buttonBackgroundColor;
		button.layer.borderWidth = borderWidth;
		button.layer.cornerRadius = cornerRadius;
		
		button.frame = CGRectMake(metalViewSize.width * 0.5f - buttonWidth / 2.0, metalViewSize.height * 0.4 - buttonHeight / 2.0, buttonWidth, buttonHeight);
		
		[button addTarget:gMainMenuHandler action:@selector(playGame) forControlEvents:UIControlEventPrimaryActionTriggered];
		
		[menuView addSubview:button];
	}
	
	{
		UIButton *button = [UIButton buttonWithType:UIButtonTypeRoundedRect];
		[button setTitle:@"Tutorial" forState:UIControlStateNormal];
		
		button.titleLabel.font = font;
		[button setTitleColor:titleColor forState:UIControlStateNormal];
		
		button.layer.backgroundColor = buttonBackgroundColor;
		button.layer.borderWidth = borderWidth;
		button.layer.cornerRadius = cornerRadius;
		
		button.frame = CGRectMake(metalViewSize.width * 0.5f - buttonWidth / 2.0, metalViewSize.height * 0.55 - buttonHeight / 2.0, buttonWidth, buttonHeight);
		
		[button addTarget:gMainMenuHandler action:@selector(playTutorial) forControlEvents:UIControlEventPrimaryActionTriggered];
		
		[menuView addSubview:button];
	}
	
	{
		UIButton *button = [UIButton buttonWithType:UIButtonTypeRoundedRect];
		[button setTitle:@"Online" forState:UIControlStateNormal];
		
		button.titleLabel.font = font;
		[button setTitleColor:titleColor forState:UIControlStateNormal];
		
		button.layer.backgroundColor = buttonBackgroundColor;
		button.layer.borderWidth = borderWidth;
		button.layer.cornerRadius = cornerRadius;
		
		button.frame = CGRectMake(metalViewSize.width * 0.5f - buttonWidth / 2.0, metalViewSize.height * 0.7 - buttonHeight / 2.0, buttonWidth, buttonHeight);
		
		[button addTarget:gMainMenuHandler action:@selector(showOnlineMenu) forControlEvents:UIControlEventPrimaryActionTriggered];
		
		[menuView addSubview:button];
	}
	
	{
		UIButton *button = [UIButton buttonWithType:UIButtonTypeRoundedRect];
		[button setTitle:@"Options" forState:UIControlStateNormal];
		
		button.titleLabel.font = font;
		[button setTitleColor:titleColor forState:UIControlStateNormal];
		
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
	CGColorRef buttonBackgroundColor = cellBackgroundColor().CGColor;
	CGFloat borderWidth = 0.0;
	CGFloat cornerRadius = 9.0;
	
	menuView.opaque = NO;
	
	{
		UIButton *button = [UIButton buttonWithType:UIButtonTypeRoundedRect];
		[button setTitle:@"Resume" forState:UIControlStateNormal];
		
		button.titleLabel.font = font;
		[button setTitleColor:titleColor forState:UIControlStateNormal];
		
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
		
		button.layer.backgroundColor = buttonBackgroundColor;
		button.layer.borderWidth = borderWidth;
		button.layer.cornerRadius = cornerRadius;
		
		button.frame = CGRectMake(metalViewSize.width * 0.5f - buttonWidth / 2.0, metalViewSize.height * 0.55 - buttonHeight / 2.0, buttonWidth, buttonHeight);
		
		[button addTarget:gPauseMenuHandler action:@selector(exitGame) forControlEvents:UIControlEventPrimaryActionTriggered];
		
		[menuView addSubview:button];
	}
	
	return menuView;
}

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
	tableView.allowsSelection = NO;
	[optionsMenu addSubview:tableView];
	
	UIButton *cancelButton = makeCancelButton(metalViewSize);
	[cancelButton addTarget:gOptionsMenuHandler action:@selector(navigateBack) forControlEvents:UIControlEventPrimaryActionTriggered];
	[optionsMenu addSubview:cancelButton];
	
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
	
	UIButton *cancelButton = makeCancelButton(metalViewSize);
	[cancelButton addTarget:gOnlineMenuHandler action:@selector(navigateBack) forControlEvents:UIControlEventPrimaryActionTriggered];
	gOnlineMenuHandler.cancelButton = cancelButton;
	[onlineMenu addSubview:cancelButton];
	
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
	
	UIButton *cancelButton = makeCancelButton(metalViewSize);
	[cancelButton addTarget:gHostGameMenuHandler action:@selector(navigateBack) forControlEvents:UIControlEventPrimaryActionTriggered];
	[view addSubview:cancelButton];
	
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
	
	UIButton *cancelButton = makeCancelButton(metalViewSize);
	[cancelButton addTarget:gJoinGameMenuHandler action:@selector(navigateBack) forControlEvents:UIControlEventPrimaryActionTriggered];
	gJoinGameMenuHandler.cancelButton = cancelButton;
	[view addSubview:cancelButton];
	
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
	pauseMusic();
	ZGUninstallTouchGestures(window);
	
	gPauseMenuHandler.resumedGameState = *gameState;
	*gameState = GAME_STATE_PAUSED;
	
	UIView *metalView = metalViewForWindow(window);
	((CAMetalLayer *)metalView.layer).presentsWithTransaction = YES;
	
	[metalView addSubview:gPauseMenuView];
}

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
