// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_PASSWORDS_NOTIFY_AUTO_SIGNIN_VIEW_CONTROLLER_H_
#define IOS_CHROME_BROWSER_PASSWORDS_NOTIFY_AUTO_SIGNIN_VIEW_CONTROLLER_H_

#import <UIKit/UIKit.h>

#import "ios/chrome/browser/ui/rtl_geometry.h"
#import "ios/chrome/browser/ui/util/constraints_ui_util.h"

// TODO(crbug.com/435048): Add UIImageView with user's avatar.

// UIViewController for the notification about auto sign-in.
@interface NotifyUserAutoSigninViewController : UIViewController

- (instancetype)initWithUserId:(NSString*)userId;

// Username, corresponding to Credential.id field in JS.
@property(assign, nonatomic) NSString* userId;

@end

#endif  // IOS_CHROME_BROWSER_PASSWORDS_NOTIFY_AUTO_SIGNIN_VIEW_CONTROLLER_H_
