// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_UI_TOOLBAR_TOOLBAR_BUTTONS_UPDATER_H_
#define IOS_CHROME_BROWSER_UI_TOOLBAR_TOOLBAR_BUTTONS_UPDATER_H_

#import <UIKit/UIKit.h>

#import "ios/chrome/browser/ui/history_popup/requirements/tab_history_positioner.h"
#import "ios/chrome/browser/ui/history_popup/requirements/tab_history_ui_updater.h"

@interface ToolbarButtonsUpdater
    : NSObject<TabHistoryPositioner, TabHistoryUIUpdater>

@property(nonatomic, strong) UIButton* backButton;
@property(nonatomic, strong) UIButton* forwardButton;

@end

#endif  // IOS_CHROME_BROWSER_UI_TOOLBAR_TOOLBAR_BUTTONS_UPDATER_H_
