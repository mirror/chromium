// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/clean/chrome/browser/ui/dialogs/java_script_dialogs/java_script_dialog_mediator.h"

#include "base/logging.h"
#include "components/strings/grit/components_strings.h"
#import "ios/clean/chrome/browser/ui/commands/java_script_dialog_commands.h"
#import "ios/clean/chrome/browser/ui/dialogs/dialog_button_configuration.h"
#import "ios/clean/chrome/browser/ui/dialogs/dialog_mediator+subclassing.h"
#import "ios/clean/chrome/browser/ui/dialogs/dialog_text_field_configuration.h"
#import "ios/clean/chrome/browser/ui/dialogs/java_script_dialogs/java_script_dialog_request.h"
#import "ios/shared/chrome/browser/ui/commands/command_dispatcher.h"
#include "ui/base/l10n/l10n_util.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

namespace {
// Dialog configuration identifiers.
NSString* const kOKButtonID = @"OK";
NSString* const kCancelButtonID = @"Cancel";
NSString* const kPromptTextFieldID = @"Prompt Text Field";
}

@interface JavaScriptDialogMediator ()<DialogDismissalCommands>

// The dismissal dispatcher
@property(nonatomic, readonly, strong) id<JavaScriptDialogDismissalCommands>
    dismissalDispatcher;
// The request passed on initializaton.
@property(nonatomic, readonly, strong) JavaScriptDialogRequest* request;

// Called when buttons are tapped to dispatch JavaScriptDialogDismissalCommands.
- (void)OKButtonWasTappedWithTextFieldValues:
    (NSDictionary<id, NSString*>*)textFieldValues;
- (void)cancelButtonWasTapped;

@end

@implementation JavaScriptDialogMediator
@synthesize dismissalDispatcher = _dismissalDispatcher;
@synthesize request = _request;

- (instancetype)initWithRequest:(JavaScriptDialogRequest*)request
                     dispatcher:
                         (id<JavaScriptDialogDismissalCommands>)dispatcher {
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
  if (buttonID == kOKButtonID) {
    [self OKButtonWasTappedWithTextFieldValues:textFieldValues];
  } else if (buttonID == kCancelButtonID) {
    [self cancelButtonWasTapped];
  } else {
    NOTREACHED() << "Received dialog dismissal for unknown button tag.";
  }
}

#pragma mark -

- (void)OKButtonWasTappedWithTextFieldValues:
    (NSDictionary<id, NSString*>*)textFieldValues {
  NSString* userInput = nil;
  if (self.request.type == web::JAVASCRIPT_DIALOG_TYPE_PROMPT) {
    userInput = textFieldValues[kPromptTextFieldID];
    userInput = userInput ? userInput : @"";
  }
  [self.request finishRequestWithSuccess:YES userInput:userInput];
  [self.dismissalDispatcher dismissJavaScriptDialog];
}

- (void)cancelButtonWasTapped {
  DCHECK_NE(self.request.type, web::JAVASCRIPT_DIALOG_TYPE_ALERT);
  [self.request finishRequestWithSuccess:NO userInput:nil];
  [self.dismissalDispatcher dismissJavaScriptDialog];
}

@end

@implementation JavaScriptDialogMediator (Internal)

- (NSString*)dialogTitle {
  return self.request.title;
}

- (NSString*)dialogMessage {
  return self.request.message;
}

- (NSArray<DialogButtonConfiguration*>*)buttonConfigs {
  NSMutableArray<DialogButtonConfiguration*>* configs =
      [[NSMutableArray alloc] init];
  // All JavaScript dialogs have an OK button.
  [configs addObject:[DialogButtonConfiguration
                         itemWithText:l10n_util::GetNSString(IDS_OK)
                                style:DialogButtonStyle::DEFAULT
                           identifier:kOKButtonID]];
  // Only confirmations and prompts get cancel buttons.
  if (self.request.type != web::JAVASCRIPT_DIALOG_TYPE_ALERT) {
    NSString* cancelText = l10n_util::GetNSString(IDS_CANCEL);
    [configs addObject:[DialogButtonConfiguration
                           itemWithText:cancelText
                                  style:DialogButtonStyle::CANCEL
                             identifier:kCancelButtonID]];
  }
  return configs;
}

- (NSArray<DialogTextFieldConfiguration*>*)textFieldConfigs {
  // Only prompts have text fields.
  if (self.request.type != web::JAVASCRIPT_DIALOG_TYPE_PROMPT)
    return nil;
  return @[ [DialogTextFieldConfiguration
      itemWithDefaultText:self.request.defaultPromptText
          placeholderText:nil
                   secure:NO
               identifier:kPromptTextFieldID] ];
}

@end
