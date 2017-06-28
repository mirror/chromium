// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_UI_CONTENT_SUGGESTIONS_CONTENT_SUGGESTIONS_HEADER_CONTROLLER_H_
#define IOS_CHROME_BROWSER_UI_CONTENT_SUGGESTIONS_CONTENT_SUGGESTIONS_HEADER_CONTROLLER_H_

#import <UIKit/UIKit.h>

// Controller for the ContentSuggestions header.
@protocol ContentSuggestionsHeaderController

// Returns whether the omnibox is currently focused.
- (BOOL)isOmniboxFocused;

// Update the iPhone fakebox's frame based on the current scroll view |offset|.
- (void)updateSearchFieldForOffset:(CGFloat)offset;

// Unfocuses the omnibox.
- (void)unfocusOmnibox;

@end

#endif  // IOS_CHROME_BROWSER_UI_CONTENT_SUGGESTIONS_CONTENT_SUGGESTIONS_HEADER_CONTROLLER_H_
