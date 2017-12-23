// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_UI_EXTERNAL_APP_EXTERNAL_APP_PRESENTER_H_
#define IOS_CHROME_BROWSER_UI_EXTERNAL_APP_EXTERNAL_APP_PRESENTER_H_

#import <Foundation/Foundation.h>

#include "ios/chrome/browser/procedural_block_types.h"

class GURL;

@protocol ExternalAppPresenter
// Opens URL in an external application if possible (optionally after
// confirming via dialog in case that user didn't interact using
// |linkTapped| or if the external application is face time) or returns NO
// if there is no such application available.
- (BOOL)openURL:(const GURL&)gurl linkTapped:(BOOL)linkTapped;

- (void)showAlertOfRepeatedLaunchesWithCompletionHandler:
    (ProceduralBlockWithBool)completionHandler;
@end

#endif  // IOS_CHROME_BROWSER_UI_EXTERNAL_APP_EXTERNAL_APP_PRESENTER_H_
