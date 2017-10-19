// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_UI_EXTERNAL_SEARCH_EXTERNAL_SEARCH_COORDINATOR_H_
#define IOS_CHROME_BROWSER_UI_EXTERNAL_SEARCH_EXTERNAL_SEARCH_COORDINATOR_H_

#import <UIKit/UIKit.h>

@class CommandDispatcher;

// Handles interaction during an External Search.
@interface ExternalSearchCoordinator : NSObject

// The dispatcher for this coordinator. When |dispatcher| is set, the
// coordinator will register itself as the target for ExternalSearchCommands.
@property(nonatomic, nullable, weak) CommandDispatcher* dispatcher;

// Stops listening for dispatcher calls and clears this coordinator's dispatcher
// property.
- (void)disconnect;

@end

#endif  // IOS_CHROME_BROWSER_UI_EXTERNAL_SEARCH_EXTERNAL_SEARCH_COORDINATOR_H_
