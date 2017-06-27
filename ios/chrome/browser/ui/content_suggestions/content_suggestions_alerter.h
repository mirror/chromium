// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_UI_CONTENT_SUGGESTIONS_CONTENT_SUGGESTIONS_ALERTER_H_
#define IOS_CHROME_BROWSER_UI_CONTENT_SUGGESTIONS_CONTENT_SUGGESTIONS_ALERTER_H_

#import <UIKit/UIKit.h>

@class AlertCoordinator;
@class CollectionViewItem;
@protocol ContentSuggestionsAlerterCommands;

@interface ContentSuggestionsAlerter : NSObject

+ (AlertCoordinator*)
alertCoordinatorForSuggestionItem:(CollectionViewItem*)item
                 onViewController:(UIViewController*)viewController
                          atPoint:(CGPoint)touchLocation
                          atIndex:(NSIndexPath*)indexPath
                   commandHandler:
                       (id<ContentSuggestionsAlerterCommands>)commandHandler;

+ (AlertCoordinator*)
alertCoordinatorForMostVisitedItem:(CollectionViewItem*)item
                  onViewController:(UIViewController*)viewController
                           atPoint:(CGPoint)touchLocation
                           atIndex:(NSIndexPath*)indexPath
                    commandHandler:
                        (id<ContentSuggestionsAlerterCommands>)commandHandler;

@end

#endif  // IOS_CHROME_BROWSER_UI_CONTENT_SUGGESTIONS_CONTENT_SUGGESTIONS_ALERTER_H_
