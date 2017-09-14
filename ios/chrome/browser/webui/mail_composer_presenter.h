// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_WEBUI_MAIL_COMPOSER_PRESENTER_H_
#define IOS_CHROME_BROWSER_WEBUI_MAIL_COMPOSER_PRESENTER_H_

#import <Foundation/Foundation.h>

@class ShowMailComposerCommand;

@protocol MailComposerPresenter<NSObject>

- (void)showMailComposer:(ShowMailComposerCommand*)command;

@end

#endif  // IOS_CHROME_BROWSER_WEBUI_MAIL_COMPOSER_PRESENTER_H_
