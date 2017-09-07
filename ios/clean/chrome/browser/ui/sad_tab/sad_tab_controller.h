// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_UI_SAD_TAB_SAD_TAB_VIEW_CONTROLLER_H_
#define IOS_CHROME_BROWSER_UI_SAD_TAB_SAD_TAB_VIEW_CONTROLLER_H_

#import <Foundation/Foundation.h>

@protocol ApplicationCommands;

namespace web {
class WebState;
}

// Controller that handles and displays a SadTabView.
@interface SadTabController : NSObject

- (instancetype)initWithWebState:(web::WebState*)webState;

// Presents a SadTabView and configures it using |repeatedFailure|.
- (void)presentSadTabForRepeatedFailure:(BOOL)repeatedFailure;

// The dispatcher for this view controller.
@property(nonatomic, weak) id<ApplicationCommands> dispatcher;

@end

#endif  // IOS_CHROME_BROWSER_UI_SAD_TAB_SAD_TAB_VIEW_CONTROLLER_H_
