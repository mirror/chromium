// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/clean/chrome/browser/ui/dialogs/dialog_mediator.h"

#include "base/logging.h"
#import "ios/clean/chrome/browser/ui/commands/dialog_commands.h"
#import "ios/clean/chrome/browser/ui/dialogs/dialog_button_configuration.h"
#import "ios/clean/chrome/browser/ui/dialogs/dialog_consumer.h"
#import "ios/clean/chrome/browser/ui/dialogs/dialog_mediator+subclassing.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

@implementation DialogMediator

#pragma mark - Public

- (void)updateConsumer:(id<DialogConsumer>)consumer {
  [consumer setDialogTitle:[self dialogTitle]];
  [consumer setDialogMessage:[self dialogMessage]];
  [consumer setDialogButtonConfigurations:[self buttonConfigs]];
  [consumer setDialogTextFieldConfigurations:[self textFieldConfigs]];
}

#pragma mark - DialogDismissalCommands

- (void)dismissDialogWithButtonID:(DialogConfigurationIdentifier*)buttonID
                  textFieldValues:
                      (NSDictionary<DialogConfigurationIdentifier*, NSString*>*)
                          textFieldValues {
  // Implemented by subclasses.
}

@end

@implementation DialogMediator (Subclassing)

- (NSString*)dialogTitle {
  // Implemented by subclasses.
  return nil;
}

- (NSString*)dialogMessage {
  // Implemented by subclasses.
  return nil;
}

- (NSArray<DialogButtonConfiguration*>*)buttonConfigs {
  // Implemented by subclasses.
  return nil;
}

- (NSArray<DialogTextFieldConfiguration*>*)textFieldConfigs {
  // Implemented by subclasses.
  return nil;
}

@end
