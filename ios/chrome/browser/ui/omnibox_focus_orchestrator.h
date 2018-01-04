// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_UI_OMNIBOX_FOCUS_ORCHESTRATOR_H_
#define IOS_CHROME_BROWSER_UI_OMNIBOX_FOCUS_ORCHESTRATOR_H_

#import <UIKit/UIKit.h>

@protocol Animatee<NSObject>

- (void)prepareToExpand;
- (void)prepareToContract;

@end

@protocol ToolbarAnimatee<Animatee>

- (void)showIncognitoButtonIfNecessary;
- (void)hideIncognitoButtonIfNecessary;

- (void)showButtons;
- (void)hideButtons;

- (void)expandLocationBarContainer;
- (void)contractLocationBarContainer;

- (void)showContractButton;
- (void)hideContractButton;

- (void)showBorder;
- (void)hideBorder;

@end

@protocol LocationBarAnimatee<Animatee>

- (void)showLeadingButtonIfNecessary;
- (void)hideLeadingButtonIfNecessary;

@end

@protocol OmniboxAnimatee<Animatee>

- (void)showClearButton;
- (void)hideClearButton;

@end

@interface OmniboxFocusOrchestrator : NSObject

@property(nonatomic, weak) id<ToolbarAnimatee> toolbarAnimatee;
@property(nonatomic, weak) id<LocationBarAnimatee> locationBarAnimatee;
@property(nonatomic, weak) id<OmniboxAnimatee> omniboxAnimatee;

- (void)expandOmnibox:(BOOL)animated;
- (void)contractOmnibox:(BOOL)animated;

@end

#endif  // IOS_CHROME_BROWSER_UI_OMNIBOX_FOCUS_ORCHESTRATOR_H_
