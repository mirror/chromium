// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_UI_CONTENT_SUGGESTIONS_CONTENT_SUGGESTIONS_ALERTER_COMMANDS_H_
#define IOS_CHROME_BROWSER_UI_CONTENT_SUGGESTIONS_CONTENT_SUGGESTIONS_ALERTER_COMMANDS_H_

#import <UIKit/UIKit.h>

@class CollectionViewItem;

@protocol ContentSuggestionsAlerterCommands

- (void)openNewTabWithSuggestionsItem:(CollectionViewItem*)item
                            incognito:(BOOL)incognito;
- (void)addItemToReadingList:(CollectionViewItem*)item;
- (void)dismissSuggestion:(CollectionViewItem*)item
              atIndexPath:(NSIndexPath*)indexPath;

- (void)openNewTabWithMostVisitedItem:(CollectionViewItem*)item
                            incognito:(BOOL)incognito
                              atIndex:(NSInteger)mostVisitedIndex;
- (void)removeMostVisited:(CollectionViewItem*)item;

@end

#endif  // IOS_CHROME_BROWSER_UI_CONTENT_SUGGESTIONS_CONTENT_SUGGESTIONS_ALERTER_COMMANDS_H_
