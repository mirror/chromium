// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_UI_SETTINGS_SETTINGS_NAVIGATION_CONTROLLER_DELEGATE_H_
#define IOS_CHROME_BROWSER_UI_SETTINGS_SETTINGS_NAVIGATION_CONTROLLER_DELEGATE_H_

#import <UIKit/UIKit.h>

#import "ios/chrome/browser/ui/commands/application_commands.h"

namespace ios {
class ChromeBrowserState;
}  // namespace ios

@protocol BrowserCommands;

@protocol SettingsNavigationControllerDelegate<NSObject>

@optional
// Informs the delegate that the settings navigation controller should be
// closed. If implemented, the delegate should close the navigation controller
// if not, the navigation controller will dismiss itself.
- (void)closeSettings;

@required
// Asks the delegate for a dispatcher that can be passed into child view
// controllers when they are created.
- (id<ApplicationCommands, BrowserCommands>)dispatcherForSettings;

@end

@protocol SettingsBrowserStateProvider

// Allows the delegate to provide a ChromeBrowserState object that may be
// used to configure the settings options.
- (ios::ChromeBrowserState*)browserStateForSettings;

@end

#endif  // IOS_CHROME_BROWSER_UI_SETTINGS_SETTINGS_NAVIGATION_CONTROLLER_DELEGATE_H_
