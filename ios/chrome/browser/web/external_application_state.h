// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_WEB_EXTERNAL_APPLICATION_STATE_H_
#define IOS_CHROME_BROWSER_WEB_EXTERNAL_APPLICATION_STATE_H_

#import <Foundation/Foundation.h>

typedef NS_ENUM(NSInteger, ExternalAppLaunchPolicy) {
  // Allow the application to launch.
  ExternalAppLaunchPolicyAllow = 0,
  // Prompt the user with a dialog, so they can choose wether to block the
  // application or launch it.
  ExternalAppLaunchPolicyPrompt,
  // Block launching the application for this session.
  ExternalAppLaunchPolicyBlock
};

// The maximum time in seconds between 2 launches of the same app to be
// considered consecutive launches.
const double kMaxSecondsBetweenConsecutiveExternalAppLaunches = 30.0;

// The maximum allowed number of consecutive launches of the same app before
// starting to prompt.
const int kMaxAllowedConsecutiveExternalAppLaunches = 2;

// ExternalApplicationState is a state for a single external application
// represented by timestamp of the last time this app was launched, and the
// number of consecutive launches. Launches are considered consecutive when the
// time difference between them are less than
// |kMaxSecondsBetweenConsecutiveExternalAppLaunches|.
// The ExternalApplicationState doesn't know the source URL nor the destination
// URL, the ExternalAppLauncher object will have an ExternalApplicationState for
// each sourceURL/Application Scheme pair.
@interface ExternalApplicationState : NSObject

// Updates the state with one more try to open the application, this method will
// check the last time the application was opened and the number of times it was
// opened consecutively then update the state.
- (void)updateStateWithLaunchRequest;

// Sets the state of the application as blocked, which means for the remaining
// lifetime of this object this app will be blocked from opening.
- (void)setStateBlocked;

// Returns the app launching recommendation based on the current state.
- (ExternalAppLaunchPolicy)launchPolicy;
@end

#endif  // IOS_CHROME_BROWSER_WEB_EXTERNAL_APP_LAUNCHING_STATE_H_
