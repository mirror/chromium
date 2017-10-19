// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_WEB_EXTERNAL_APP_LAUNCHING_STATE_H_
#define IOS_CHROME_BROWSER_WEB_EXTERNAL_APP_LAUNCHING_STATE_H_

#import <Foundation/Foundation.h>

typedef NS_ENUM(NSInteger, LaunchAction) { Allow, Prompt, Block };

// ExternalAppLaunchingState is represented by timestamp of the last time the
// app launched, and the number of consecutive launches. Launches are considered
// consecutive when the time difference between them are less than 30 seconds.
// The ExternalAppLaunchingState doesn't know the source URL nor the destination
// URL, the externalAppLauncher object is responsible to have an object from
// ExternalAppLaunchingState for each sourceURL/Application Scheme pair.
@interface ExternalAppLaunchingState : NSObject

// Update the state with one more try to open the application, this method will
// check the last time the application was opened and the number of times it was
// opened consecutively then update the state.
- (void)update;

// Set the state of the application as blocked, which means for the remaining
// lifetime of this object this app will be blocked from opening.
- (void)block;

// Returns the app launching recommendation based on the current state.
- (LaunchAction)launchAction;
@end

#endif  // IOS_CHROME_BROWSER_WEB_EXTERNAL_APP_LAUNCHING_STATE_H_
