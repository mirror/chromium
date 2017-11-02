// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_UI_ANIMATIONS_LOCATIONBAR_ANIMATION_CONFIGURATOR_H_
#define IOS_CHROME_BROWSER_UI_ANIMATIONS_LOCATIONBAR_ANIMATION_CONFIGURATOR_H_

#import <UIKit/UIKit.h>

@interface LocationbarAnimationConfigurator : NSObject

//
@property(nonatomic, assign, getter=isExpanded) BOOL expanded;

@property(nonatomic, strong) NSMutableArray* fadingLeadingButtons;

@property(nonatomic, strong) NSMutableArray* fadingTrailingButtons;

@property(nonatomic, strong) NSMutableArray* fadingLeadingViews;

@property(nonatomic, strong) NSMutableArray* fadingTrailingViews;

@property(nonatomic, weak) UIView* animatingBackground;

@property(nonatomic, weak) UIView* animatingLocationBar;

@property(nonatomic, assign) CGRect finalBackgroundFrame;

@property(nonatomic, assign) CGRect finalLocationBarFrame;

@property(nonatomic, weak) UIImageView* incognitoIcon;

// UIViewPropertyAnimator for animating the location bar.
@property(nonatomic, strong, readonly)
    UIViewPropertyAnimator* locationbarAnimator API_AVAILABLE(ios(10.0));

- (void)runAnimation;

@end

#endif  // IOS_CHROME_BROWSER_UI_ANIMATIONS_LOCATIONBAR_ANIMATION_CONFIGURATOR_H_
