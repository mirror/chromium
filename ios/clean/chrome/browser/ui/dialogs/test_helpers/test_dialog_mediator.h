// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CLEAN_CHROME_BROWSER_UI_DIALOGS_TEST_HELPERS_TEST_DIALOG_MEDIATOR_H_
#define IOS_CLEAN_CHROME_BROWSER_UI_DIALOGS_TEST_HELPERS_TEST_DIALOG_MEDIATOR_H_

#import "ios/clean/chrome/browser/ui/dialogs/dialog_mediator.h"

@class DialogButtonConfiguration;
@class DialogTextFieldConfiguration;

// A DialogMediator subclass that can be configured via properties.
@interface TestDialogMediator : DialogMediator
@property(nonatomic, strong) NSString* title;
@property(nonatomic, strong) NSString* message;
@property(nonatomic, strong) NSArray<DialogButtonConfiguration*>* buttonConfigs;
@property(nonatomic, strong)
    NSArray<DialogTextFieldConfiguration*>* textFieldConfigs;
@end

#endif  // IOS_CLEAN_CHROME_BROWSER_UI_DIALOGS_TEST_HELPERS_TEST_DIALOG_MEDIATOR_H_
