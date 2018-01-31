// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_UI_ORCHESTRATOR_OMNIBOX_FOCUS_ORCHESTRATOR_H_
#define IOS_CHROME_BROWSER_UI_ORCHESTRATOR_OMNIBOX_FOCUS_ORCHESTRATOR_H_

#import <UIKit/UIKit.h>

@protocol ToolbarAnimatee;

@interface OmniboxFocusOrchestrator : NSObject

@property(nonatomic, weak) id<ToolbarAnimatee> toolbarAnimatee;

- (void)expandOmnibox:(BOOL)animated;
- (void)contractOmnibox:(BOOL)animated;

@end

#endif  // IOS_CHROME_BROWSER_UI_ORCHESTRATOR_OMNIBOX_FOCUS_ORCHESTRATOR_H_
