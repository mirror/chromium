// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_UI_ACTIVITY_SERVICES_SHARE_PROTOCOL_H_
#define IOS_CHROME_BROWSER_UI_ACTIVITY_SERVICES_SHARE_PROTOCOL_H_

#import <Foundation/Foundation.h>
#import <UIKit/UIKit.h>

@protocol ActivityServicePassword;
@protocol ActivityServicePositioner;
@protocol ActivityServicePresentation;
@protocol ActivityServiceSnackbar;
@class ShareToData;

namespace ios {
class ChromeBrowserState;
}

namespace ShareTo {

// Provides the result of a sharing event.
enum ShareResult {
  // The share was successful.
  SHARE_SUCCESS,
  // The share was cancelled by either the user or by the service.
  SHARE_CANCEL,
  // The share was attempted, but failed due to a network-related error.
  SHARE_NETWORK_FAILURE,
  // The share was attempted, but failed because of user authentication error.
  SHARE_SIGN_IN_FAILURE,
  // The share was attempted, but failed with an unspecified error.
  SHARE_ERROR,
  // The share was attempted, and the result is unknown.
  SHARE_UNKNOWN_RESULT,
};

}  // namespace ShareTo

@protocol ShareProtocol<NSObject>

// Returns YES if a share is currently in progress.
- (BOOL)isActive;

// Cancels share operation.
- (void)cancelShareAnimated:(BOOL)animated;

// Shares the given data. The controller should be the current controller and
// will be used to layer the necessary UI on top of it. The delegate will
// receive callbacks about sharing events. On iPad, |rect| specifies the anchor
// location for the UIPopover relative to |inView|.
// |-isEnabled| must be YES. |data|, |controller|, |delegate| must not be nil.
// When sharing to iPad, |inView| must not be nil and |rect|'s size must not be
// zero.
- (void)shareWithData:(ShareToData*)data
              controller:(UIViewController*)controller
            browserState:(ios::ChromeBrowserState*)browserState
        passwordProvider:(id<ActivityServicePassword>)passwordProvider
        positionProvider:(id<ActivityServicePositioner>)positionProvider
    presentationProvider:(id<ActivityServicePresentation>)presentationProvider
        snackbarProvider:(id<ActivityServiceSnackbar>)snackbarProvider;
@end

#endif  // IOS_CHROME_BROWSER_UI_ACTIVITY_SERVICES_SHARE_PROTOCOL_H_
