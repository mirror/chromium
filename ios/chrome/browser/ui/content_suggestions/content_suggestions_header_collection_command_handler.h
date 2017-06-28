// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_UI_CONTENT_SUGGESTIONS_CONTENT_SUGGESTIONS_HEADER_COLLECTION_COMMAND_HANDLER_H_
#define IOS_CHROME_BROWSER_UI_CONTENT_SUGGESTIONS_CONTENT_SUGGESTIONS_HEADER_COLLECTION_COMMAND_HANDLER_H_

#import "ios/chrome/browser/ui/content_suggestions/content_suggestions_collection_commands.h"
#import "ios/chrome/browser/ui/content_suggestions/content_suggestions_header_commands.h"

#import <UIKit/UIKit.h>

#import "base/ios/block_types.h"

@class ContentSuggestionsViewController;
@protocol ContentSuggestionsHeaderController;
@protocol ContentSuggestionsViewControllerDelegate;

// Command handler for all the interactions between the HeaderController and the
// CollectionView. It handles the interactions both ways.
@interface ContentSuggestionsHeaderCollectionCommandHandler
    : NSObject<ContentSuggestionsHeaderCommands,
               ContentSuggestionsCollectionCommands>

// Initializes the CommandHandler with the |suggestionsViewController| and the
// |headerController|.
- (nullable instancetype)
initWithSuggestionsViewController:
    (nullable ContentSuggestionsViewController*)suggestionsViewController
                 headerController:
                     (nullable id<ContentSuggestionsHeaderController>)
                         headerController NS_DESIGNATED_INITIALIZER;

- (nullable instancetype)init NS_UNAVAILABLE;

@property(nonatomic, weak, nullable)
    id<ContentSuggestionsViewControllerDelegate>
        delegate;

@end

#endif  // IOS_CHROME_BROWSER_UI_CONTENT_SUGGESTIONS_CONTENT_SUGGESTIONS_HEADER_COLLECTION_COMMAND_HANDLER_H_
