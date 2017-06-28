// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_UI_CONTENT_SUGGESTIONS_CONTENT_SUGGESTIONS_COLLECTION_COMMANDS_H_
#define IOS_CHROME_BROWSER_UI_CONTENT_SUGGESTIONS_CONTENT_SUGGESTIONS_COLLECTION_COMMANDS_H_

#import "base/ios/block_types.h"

// Command protocol used by the ContentSuggestions header controller to send
// commands to the ContentSuggestions collection.
@protocol ContentSuggestionsCollectionCommands

// Updates the collection view's scroll view offset for the next frame of the
// shiftTilesDown animation.
- (void)shiftTilesDown;
// Completion called only if scrolled to top.
- (void)shiftTilesUpWithCompletionBlock:(ProceduralBlock)completionBlock;

@end

#endif  // IOS_CHROME_BROWSER_UI_CONTENT_SUGGESTIONS_CONTENT_SUGGESTIONS_COLLECTION_COMMANDS_H_
