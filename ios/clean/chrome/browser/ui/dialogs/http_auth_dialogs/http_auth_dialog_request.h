// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CLEAN_CHROME_BROWSER_UI_DIALOGS_HTTP_AUTH_DIALOGS_HTTP_AUTH_DIALOG_REQUEST_H_
#define IOS_CLEAN_CHROME_BROWSER_UI_DIALOGS_HTTP_AUTH_DIALOGS_HTTP_AUTH_DIALOG_REQUEST_H_

#import <Foundation/Foundation.h>

namespace web {
class WebState;
}

// Block type for HTTP authentication callbacks.
typedef void (^HTTPAuthDialogCallback)(NSString* username, NSString* password);

// A container object encapsulating all the state necessary to support an
// HTTPAuthDialogCoordiantor.  This object also owns the WebKit
// completion block that will throw an exception if it is deallocated before
// being executed. |-runCallbackWithUserName:password:| must be executed once in
// the lifetime of every HTTPAuthDialogRequest.
@interface HTTPAuthDialogRequest : NSObject

// Factory method to create HTTPAuthDialogRequests from the given input.  All
// arguments to this function are expected to be nonnull.
+ (instancetype)stateWithWebState:(web::WebState*)webState
                  protectionSpace:(NSURLProtectionSpace*)protectionSpace
                       credential:(NSURLCredential*)credential
                         callback:(HTTPAuthDialogCallback)callback;

// The WebState displaying this dialog.
@property(nonatomic, readonly) web::WebState* webState;

// The title to use for the dialog.
@property(nonatomic, readonly, strong) NSString* title;

// The authentication message for the dialog's protection space.
@property(nonatomic, readonly, strong) NSString* message;

// The default text to display in the username text field.
@property(nonatomic, readonly, strong) NSString* defaultUserNameText;

// Completes the HTTP authentication flow with the given username and password.
// If the user taps the OK button, |username| and |password| are expected to be
// non-nil, even if they are empty strings.  If the user taps the Cancel button,
// arguments should be nil.
- (void)completeAuthenticationWithUsername:(NSString*)username
                                  password:(NSString*)password;

@end

#endif  // IOS_CLEAN_CHROME_BROWSER_UI_DIALOGS_HTTP_AUTH_DIALOGS_HTTP_AUTH_DIALOG_REQUEST_H_
