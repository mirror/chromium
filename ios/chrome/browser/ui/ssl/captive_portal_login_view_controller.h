// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_UI_SSL_CAPTIVE_PORTAL_LOGIN_VIEW_CONTROLLER_H_
#define IOS_CHROME_BROWSER_UI_SSL_CAPTIVE_PORTAL_LOGIN_VIEW_CONTROLLER_H_

#import <UIKit/UIKit.h>

// View controller which displays a web view used to complete the connection to
// a captive portal network.
@interface CaptivePortalLoginViewController : UIViewController

// Initializes the login web view and navigates to |landingURL|. |landingURL|
// is the web page which allows the user to complete their connection to the
// network.
- (instancetype)initWithLandingURL:(NSURL*)landingURL;

- (instancetype)init NS_UNAVAILABLE;

@end

#endif  // IOS_CHROME_BROWSER_UI_SSL_CAPTIVE_PORTAL_LOGIN_VIEW_CONTROLLER_H_
