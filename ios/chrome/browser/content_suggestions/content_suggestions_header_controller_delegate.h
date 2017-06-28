// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_CONTENT_SUGGESTIONS_CONTENT_SUGGESTIONS_HEADER_CONTROLLER_DELEGATE_H_
#define IOS_CHROME_BROWSER_CONTENT_SUGGESTIONS_CONTENT_SUGGESTIONS_HEADER_CONTROLLER_DELEGATE_H_

#import <UIKit/UIKit.h>

#import "base/ios/block_types.h"

@protocol ContentSuggestionsHeaderControllerDelegate

- (BOOL)isContextMenuVisible;
- (BOOL)isScrolledToTop;

@end

@protocol ContentSuggestionsHeaderControllerCommandHandler

// TODO: one protocol to allow the header controller to send commands to the
// collection, another to allow the collection to send commands to the header
// controller. They should be implemented in the same object.
- (void)dismissModals;

@end

#endif  // IOS_CHROME_BROWSER_CONTENT_SUGGESTIONS_CONTENT_SUGGESTIONS_HEADER_CONTROLLER_DELEGATE_H_
