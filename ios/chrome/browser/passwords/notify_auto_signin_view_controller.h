// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_PASSWORDS_NOTIFY_AUTO_SIGNIN_VIEW_CONTROLLER_H_
#define IOS_CHROME_BROWSER_PASSWORDS_NOTIFY_AUTO_SIGNIN_VIEW_CONTROLLER_H_

#import <UIKit/UIKit.h>

// TODO(crbug.com/435048): Add UIImageView with user's avatar.

// UIViewController for the notification about auto sign-in.
@interface NotifyUserAutoSigninViewController : UIViewController

- (instancetype)initWithUserID:(NSString*)userID NS_DESIGNATED_INITIALIZER;

- (instancetype)initWithNibName:(NSString*)nibNameOrNil
                         bundle:(NSBundle*)nibBundleOrNil NS_UNAVAILABLE;
- (instancetype)initWithCoder:(NSCoder*)aDecoder NS_UNAVAILABLE;

// Username, corresponding to Credential.id field in JS.
@property(assign, nonatomic) NSString* userID;

@end

#endif  // IOS_CHROME_BROWSER_PASSWORDS_NOTIFY_AUTO_SIGNIN_VIEW_CONTROLLER_H_
