//
//  window_scene_ios.h
//  SkyCheckers
//
//  Created by Mayur Pawashe on 4/6/25.
//

#import <UIKit/UIKit.h>
#include "window.h"

NS_ASSUME_NONNULL_BEGIN

@interface ZGGameSceneDelegate : UIResponder <UIWindowSceneDelegate>

@property (nonatomic, nullable) UIWindow *window;

@property (nonatomic, nullable) void (*windowEventHandler)(ZGWindowEvent, void *);
@property (nonatomic, nullable) void *windowEventHandlerContext;

@end


@interface ZGViewController : UIViewController <UIGestureRecognizerDelegate>

- (instancetype)initWithFrame:(CGRect)frame;

@end

NS_ASSUME_NONNULL_END
