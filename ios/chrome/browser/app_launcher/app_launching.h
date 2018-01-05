// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_APP_LAUNCHER_APP_LAUNCHING_H_
#define IOS_CHROME_BROWSER_APP_LAUNCHER_APP_LAUNCHING_H_

#include "ios/chrome/browser/procedural_block_types.h"

class GURL;

// Protocol for handling application launching.
@protocol AppLaunching

// Launches application that has |URL| if possible (optionally after confirming
// via dialog in case the user didn't interact using |linkTapped| or if the
// application is facetime). Returns NO if there is no such application
// available.
- (BOOL)launchAppWithURL:(const GURL&)URL linkTapped:(BOOL)linkTapped;

// Alerts the user that there have been repeated attempts to launch
// the application. |completionHandler| is called with the user's
// response on whether to launch the application.
- (void)showAlertOfRepeatedLaunchesWithCompletionHandler:
    (ProceduralBlockWithBool)completionHandler;

@end

#endif  // IOS_CHROME_BROWSER_APP_LAUNCHER_APP_LAUNCHING_H_
