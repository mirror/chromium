// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_WEB_OPEN_MAIL_HANDLER_VIEW_CONTROLLER_H_
#define IOS_CHROME_BROWSER_WEB_OPEN_MAIL_HANDLER_VIEW_CONTROLLER_H_

#include "ios/chrome/browser/ui/collection_view/collection_view_controller.h"

@class MailtoURLRewriter;
@class UICollectionViewLayout;

@interface OpenMailHandlerViewController : CollectionViewController
- (instancetype)initWithRewriter:(MailtoURLRewriter*)rewriter
                          layout:(UICollectionViewLayout*)layout
                           style:(CollectionViewControllerStyle)style;
@end

#endif  // IOS_CHROME_BROWSER_WEB_OPEN_MAIL_HANDLER_VIEW_CONTROLLER_H_
