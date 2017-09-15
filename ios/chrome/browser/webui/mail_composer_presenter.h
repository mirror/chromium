// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_WEBUI_MAIL_COMPOSER_PRESENTER_H_
#define IOS_CHROME_BROWSER_WEBUI_MAIL_COMPOSER_PRESENTER_H_

#import <Foundation/Foundation.h>

@class ShowMailComposerCommand;

// Protocol for objects that can display UI to send emails.
@protocol MailComposerPresenter<NSObject>

// Shows the Mail Composer UI. |context| provides information to populate the
// email.
- (void)showMailComposer:(ShowMailComposerCommand*)context;

@end

#endif  // IOS_CHROME_BROWSER_WEBUI_MAIL_COMPOSER_PRESENTER_H_
