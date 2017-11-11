// Copyright 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_WEB_EXTERNAL_APP_LAUNCHER_H_
#define IOS_CHROME_BROWSER_WEB_EXTERNAL_APP_LAUNCHER_H_

#import <UIKit/UIKit.h>

class GURL;

// A customized external app launcher that optionally shows a modal
// confirmation dialog before switching context to an external application.
@interface ExternalAppLauncher : NSObject

// Requests to open URL in an external application. The method checks if the
// application for |gURL| has been opened repeatedly by the |sourcePageURL|
// page within a short time frame. In that case, user will be prompted with an
// option to block the application from launching.
// Then the method also checks for user interaction and for schemes that require
// special handling (eg. facetime, mailto) and may present the user with a
// confirmation dialog to open the application.
// Always calls |completionHandler|, which must not be nil, on the main thread
// with status of whether an app was launched for opening |gURL|.
- (void)requestToOpenURL:(const GURL&)gURL
           sourcePageURL:(const GURL&)sourcePageURL
             linkClicked:(BOOL)linkClicked
       completionHandler:(void (^_Nonnull)(BOOL))completionHandler;

@end

#endif  // IOS_CHROME_BROWSER_WEB_EXTERNAL_APP_LAUNCHER_H_
