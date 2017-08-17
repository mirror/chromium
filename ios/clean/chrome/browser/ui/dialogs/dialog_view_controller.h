// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CLEAN_CHROME_BROWSER_UI_DIALOGS_DIALOG_VIEW_CONTROLLER_H_
#define IOS_CLEAN_CHROME_BROWSER_UI_DIALOGS_DIALOG_VIEW_CONTROLLER_H_

#import <UIKit/UIKit.h>

#import "ios/clean/chrome/browser/ui/dialogs/dialog_consumer.h"

@protocol DialogDismissalCommands;

// Class used to display dialogs.  It is configured through the DialogConsumer
// protocol, and is not meant to be subclassed.
@interface DialogViewController : UIAlertController<DialogConsumer>

// Initializer for a dialog with |style| that uses |dispatcher| to manage its
// dismissal.
- (instancetype)initWithStyle:(UIAlertControllerStyle)style
                   dispatcher:(id<DialogDismissalCommands>)dispatcher;

@end

#endif  // IOS_CLEAN_CHROME_BROWSER_UI_DIALOGS_DIALOG_VIEW_CONTROLLER_H_
