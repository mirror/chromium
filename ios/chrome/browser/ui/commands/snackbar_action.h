// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_UI_COMMANDS_SNACKBAR_ACTION_H_
#define IOS_CHROME_BROWSER_UI_COMMANDS_SNACKBAR_ACTION_H_

#import <Foundation/Foundation.h>

#include "base/ios/block_types.h"

// Encapsulates an action for the snackbar.
@interface SnackbarAction : NSObject
// Title for the action button.
@property(nonatomic, copy) NSString* title;
// Accessibility identifier for the action button.
@property(nonatomic, copy) NSString* accessibilityIdentifier;
// Accessibility label for the action button.
@property(nonatomic, copy) NSString* accessibilityLabel;
// Block executed on tap of the action button.
@property(nonatomic, copy) ProceduralBlock handler;
@end

#endif  // IOS_CHROME_BROWSER_UI_COMMANDS_SNACKBAR_ACTION_H_
