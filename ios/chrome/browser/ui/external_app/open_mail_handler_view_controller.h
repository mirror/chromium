// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_UI_EXTERNAL_APP_OPEN_MAIL_HANDLER_VIEW_CONTROLLER_H_
#define IOS_CHROME_BROWSER_UI_EXTERNAL_APP_OPEN_MAIL_HANDLER_VIEW_CONTROLLER_H_

#include "ios/chrome/browser/ui/collection_view/collection_view_controller.h"

@class MailtoHandler;
@class MailtoURLRewriter;

// Type of callback block when user selects a mailto:// handler.
typedef void (^OpenMailtoHandlerSelectedCallback)(MailtoHandler* handler);

// A view controller object to prompt user to make a selection for which app
// to use to handle a tap on mailto:// URL.
@interface OpenMailHandlerViewController : CollectionViewController

// Optional callback block that will be called after user has selected a
// mailto:// handler.
@property(nonatomic, strong)
    OpenMailtoHandlerSelectedCallback onMailtoHandlerSelected;

// Initializes the view controller with the |rewriter| object which supplies
// the list of supported apps that can handle a mailto:// URL.
- (instancetype)initWithRewriter:(MailtoURLRewriter*)rewriter;
@end

#endif  // IOS_CHROME_BROWSER_UI_EXTERNAL_APP_OPEN_MAIL_HANDLER_VIEW_CONTROLLER_H_
