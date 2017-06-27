// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_UI_CONTENT_SUGGESTIONS_CONTENT_SUGGESTIONS_ALERTER_COMMANDS_H_
#define IOS_CHROME_BROWSER_UI_CONTENT_SUGGESTIONS_CONTENT_SUGGESTIONS_ALERTER_COMMANDS_H_

#import <UIKit/UIKit.h>

@class CollectionViewItem;

// Command protocol for the ContentSuggestionsAlerter, handling the callbacks
// from the alerts.
@protocol ContentSuggestionsAlerterCommands

// Open the URL corresponding to the |item| in a new tab, |incognito| or not.
// The item has to a suggestion item.
- (void)openNewTabWithSuggestionsItem:(CollectionViewItem*)item
                            incognito:(BOOL)incognito;
// Adds the |item| to the reading list. The item has to a suggestion item.
- (void)addItemToReadingList:(CollectionViewItem*)item;
// Dismiss the |item| at |indexPath|. The item has to a suggestion item.
- (void)dismissSuggestion:(CollectionViewItem*)item
              atIndexPath:(NSIndexPath*)indexPath;

// Open the URL corresponding to the |item| in a new tab, |incognito| or not.
// The item has to a Most Visited item.
- (void)openNewTabWithMostVisitedItem:(CollectionViewItem*)item
                            incognito:(BOOL)incognito
                              atIndex:(NSInteger)mostVisitedIndex;
// Removes the most visited |item|.
- (void)removeMostVisited:(CollectionViewItem*)item;

@end

#endif  // IOS_CHROME_BROWSER_UI_CONTENT_SUGGESTIONS_CONTENT_SUGGESTIONS_ALERTER_COMMANDS_H_
