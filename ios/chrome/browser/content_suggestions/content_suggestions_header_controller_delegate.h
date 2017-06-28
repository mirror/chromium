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

// Dismisses all presented modals.
- (void)dismissModals;

@end

#endif  // IOS_CHROME_BROWSER_CONTENT_SUGGESTIONS_CONTENT_SUGGESTIONS_HEADER_CONTROLLER_DELEGATE_H_
