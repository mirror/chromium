// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/clean/chrome/browser/ui/dialogs/http_auth_dialogs/http_auth_dialog_mediator.h"

#include "base/logging.h"
#include "components/strings/grit/components_strings.h"
#include "ios/chrome/grit/ios_strings.h"
#import "ios/clean/chrome/browser/ui/commands/http_auth_dialog_commands.h"
#import "ios/clean/chrome/browser/ui/dialogs/dialog_button_configuration.h"
#import "ios/clean/chrome/browser/ui/dialogs/dialog_consumer.h"
#import "ios/clean/chrome/browser/ui/dialogs/dialog_mediator+subclassing.h"
#import "ios/clean/chrome/browser/ui/dialogs/dialog_text_field_configuration.h"
#import "ios/clean/chrome/browser/ui/dialogs/http_auth_dialogs/http_auth_dialog_request.h"
#import "ios/shared/chrome/browser/ui/commands/command_dispatcher.h"
#include "ui/base/l10n/l10n_util.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

namespace {
// Dialog configuration tags.
NSString* const kOKButtonID = @"OKButtonID";
NSString* const kCancelButtonID = @"CancelButtonID";
NSString* const kUsernameTextFieldID = @"UsernameTextFieldID";
NSString* const kPasswordTextFieldID = @"PasswordTextFieldID";
}

@interface HTTPAuthDialogMediator ()

// The dispatcher for dismissal.
@property(nonatomic, readonly, strong) id<HTTPAuthDialogDismissalCommands>
    dismissalDispatcher;
// The request for this dialog.
@property(nonatomic, readonly, strong) HTTPAuthDialogRequest* request;

@end

@implementation HTTPAuthDialogMediator
@synthesize dismissalDispatcher = _dismissalDispatcher;
@synthesize request = _request;

- (instancetype)initWithRequest:(HTTPAuthDialogRequest*)request
                     dispatcher:
                         (id<HTTPAuthDialogDismissalCommands>)dispatcher {
  DCHECK(request);
  DCHECK(dispatcher);
  if ((self = [super init])) {
    _request = request;
    _dismissalDispatcher = dispatcher;
  }
  return self;
}

#pragma mark - DialogDismissalCommands

- (void)dismissDialogWithButtonID:(id)buttonID
                  textFieldValues:
                      (NSDictionary<id, NSString*>*)textFieldValues {
  NSString* username = nil;
  NSString* password = nil;
  if (buttonID == kOKButtonID) {
    username = textFieldValues[kUsernameTextFieldID];
    username = username ? username : @"";
    password = textFieldValues[kPasswordTextFieldID];
    password = password ? password : @"";
  } else if (buttonID == kCancelButtonID) {
    // Use nil in callbacks for cancelled HTTP auth dialogs.
  } else {
    NOTREACHED() << "Received dialog dismissal for unknown button ID.";
  }
  [self.request completeAuthenticationWithUsername:username password:password];
  [self.dismissalDispatcher dismissHTTPAuthDialog];
}

@end

@implementation HTTPAuthDialogMediator (Subclassing)

- (NSString*)dialogTitle {
  return self.request.title;
}

- (NSString*)dialogMessage {
  return self.request.message;
}

- (NSArray<DialogButtonConfiguration*>*)buttonConfigs {
  return @[
    [DialogButtonConfiguration itemWithText:l10n_util::GetNSString(IDS_OK)
                                      style:DialogButtonStyle::DEFAULT
                                 identifier:kOKButtonID],
    [DialogButtonConfiguration itemWithText:l10n_util::GetNSString(IDS_CANCEL)
                                      style:DialogButtonStyle::CANCEL
                                 identifier:kCancelButtonID],
  ];
}

- (NSArray<DialogTextFieldConfiguration*>*)textFieldConfigs {
  NSString* defaultUsername = self.request.defaultUserNameText;
  NSString* usernamePlaceholder =
      l10n_util::GetNSString(IDS_IOS_HTTP_LOGIN_DIALOG_USERNAME_PLACEHOLDER);
  NSString* passwordPlaceholder =
      l10n_util::GetNSString(IDS_IOS_HTTP_LOGIN_DIALOG_PASSWORD_PLACEHOLDER);
  return @[
    [DialogTextFieldConfiguration itemWithDefaultText:defaultUsername
                                      placeholderText:usernamePlaceholder
                                               secure:NO
                                           identifier:kUsernameTextFieldID],
    [DialogTextFieldConfiguration itemWithDefaultText:nil
                                      placeholderText:passwordPlaceholder
                                               secure:YES
                                           identifier:kPasswordTextFieldID],
  ];
}

@end
