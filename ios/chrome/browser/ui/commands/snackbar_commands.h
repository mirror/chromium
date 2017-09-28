// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_UI_COMMANDS_SNACKBAR_COMMANDS_H_
#define IOS_CHROME_BROWSER_UI_COMMANDS_SNACKBAR_COMMANDS_H_

#import <Foundation/Foundation.h>

@class SnackbarAction;

// Commands related to Snackbar.
@protocol SnackbarCommands

// Shows a snackbar with |message|. Snackbar auto-dismisses after a short
// period. This method uses a default category. Only the last message of the
// default category will be shown. Any currently showing or pending message of
// the default category will be dismissed.
- (void)showSnackbarWithMessage:(NSString*)message;

// Shows a snackbar with |message|. Snackbar auto-dismisses after a short
// period. This method uses a custom category. Only the last message of
// |category| will be shown. Any currently showing or pending message of
// |category| will be dismissed.
- (void)showSnackbarWithMessage:(NSString*)message category:(NSString*)category;

// Shows a snackbar with |message|. Snackbar auto-dismisses after |duration|.
// This method uses a custom category. Only the last message of |category| will
// be shown. Any currently showing or pending message of |category| will be
// dismissed.
- (void)showSnackbarWithMessage:(NSString*)message
                       category:(NSString*)category
                       duration:(NSTimeInterval)duration;

// Shows a snackbar with |message|, |category|, and |action|. This method uses
// a custom category. Only the last message of |category| will be shown. Any
// currently showing or pending message of |category| will be dismissed.
- (void)showSnackbarWithMessage:(NSString*)message
                       category:(NSString*)category
                         action:(SnackbarAction*)action;

// Dismisses all snackbars of |category|. If |category| is nil, all snackbars
// are dismissed.
- (void)dismissAllSnackbarsWithCategory:(NSString*)category;

@end

#endif  // IOS_CHROME_BROWSER_UI_COMMANDS_SNACKBAR_COMMANDS_H_
