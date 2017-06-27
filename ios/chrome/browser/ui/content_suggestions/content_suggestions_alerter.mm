// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/ui/content_suggestions/content_suggestions_alerter.h"

#import "ios/chrome/browser/ui/alert_coordinator/action_sheet_coordinator.h"
#import "ios/chrome/browser/ui/collection_view/cells/collection_view_item.h"
#import "ios/chrome/browser/ui/content_suggestions/content_suggestions_alerter_commands.h"
#include "ios/chrome/grit/ios_strings.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/strings/grit/ui_strings.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

@implementation ContentSuggestionsAlerter

+ (AlertCoordinator*)
alertCoordinatorForSuggestionItem:(CollectionViewItem*)item
                 onViewController:(UIViewController*)viewController
                          atPoint:(CGPoint)touchLocation
                          atIndex:(NSIndexPath*)indexPath
                   commandHandler:
                       (id<ContentSuggestionsAlerterCommands>)commandHandler {
  AlertCoordinator* alertCoordinator = [[ActionSheetCoordinator alloc]
      initWithBaseViewController:viewController
                           title:nil
                         message:nil
                            rect:CGRectMake(touchLocation.x, touchLocation.y, 0,
                                            0)
                            view:[viewController view]];

  __weak CollectionViewItem* weakItem = item;
  __weak id<ContentSuggestionsAlerterCommands> weakCommandHandler =
      commandHandler;

  NSString* openInNewTabTitle =
      l10n_util::GetNSString(IDS_IOS_CONTENT_CONTEXT_OPENLINKNEWTAB);
  [alertCoordinator addItemWithTitle:openInNewTabTitle
                              action:^{
                                // TODO(crbug.com/691979): Add metrics.
                                [weakCommandHandler
                                    openNewTabWithSuggestionsItem:weakItem
                                                        incognito:NO];
                              }
                               style:UIAlertActionStyleDefault];

  NSString* openInNewTabIncognitoTitle =
      l10n_util::GetNSString(IDS_IOS_CONTENT_CONTEXT_OPENLINKNEWINCOGNITOTAB);
  [alertCoordinator addItemWithTitle:openInNewTabIncognitoTitle
                              action:^{
                                // TODO(crbug.com/691979): Add metrics.
                                [weakCommandHandler
                                    openNewTabWithSuggestionsItem:weakItem
                                                        incognito:YES];
                              }
                               style:UIAlertActionStyleDefault];

  NSString* readLaterTitle =
      l10n_util::GetNSString(IDS_IOS_CONTENT_CONTEXT_ADDTOREADINGLIST);
  [alertCoordinator
      addItemWithTitle:readLaterTitle
                action:^{
                  // TODO(crbug.com/691979): Add metrics.
                  [weakCommandHandler addItemToReadingList:weakItem];
                }
                 style:UIAlertActionStyleDefault];

  NSString* deleteTitle =
      l10n_util::GetNSString(IDS_IOS_CONTENT_SUGGESTIONS_REMOVE);
  [alertCoordinator addItemWithTitle:deleteTitle
                              action:^{
                                // TODO(crbug.com/691979): Add metrics.
                                [weakCommandHandler
                                    dismissSuggestion:weakItem
                                          atIndexPath:indexPath];
                              }
                               style:UIAlertActionStyleDestructive];

  [alertCoordinator addItemWithTitle:l10n_util::GetNSString(IDS_APP_CANCEL)
                              action:^{
                                // TODO(crbug.com/691979): Add metrics.
                              }
                               style:UIAlertActionStyleCancel];
  return alertCoordinator;
}

+ (AlertCoordinator*)
alertCoordinatorForMostVisitedItem:(CollectionViewItem*)item
                  onViewController:(UIViewController*)viewController
                           atPoint:(CGPoint)touchLocation
                           atIndex:(NSIndexPath*)indexPath
                    commandHandler:
                        (id<ContentSuggestionsAlerterCommands>)commandHandler {
  AlertCoordinator* alertCoordinator = [[ActionSheetCoordinator alloc]
      initWithBaseViewController:viewController
                           title:nil
                         message:nil
                            rect:CGRectMake(touchLocation.x, touchLocation.y, 0,
                                            0)
                            view:[viewController view]];

  __weak CollectionViewItem* weakItem = item;
  __weak id<ContentSuggestionsAlerterCommands> weakCommandHandler =
      commandHandler;

  [alertCoordinator
      addItemWithTitle:l10n_util::GetNSStringWithFixup(
                           IDS_IOS_CONTENT_CONTEXT_OPENLINKNEWTAB)
                action:^{
                  [weakCommandHandler
                      openNewTabWithMostVisitedItem:weakItem
                                          incognito:NO
                                            atIndex:indexPath.item];
                }
                 style:UIAlertActionStyleDefault];

  [alertCoordinator
      addItemWithTitle:l10n_util::GetNSStringWithFixup(
                           IDS_IOS_CONTENT_CONTEXT_OPENLINKNEWINCOGNITOTAB)
                action:^{
                  [weakCommandHandler
                      openNewTabWithMostVisitedItem:weakItem
                                          incognito:YES
                                            atIndex:indexPath.item];
                }
                 style:UIAlertActionStyleDefault];

  [alertCoordinator addItemWithTitle:l10n_util::GetNSStringWithFixup(
                                         IDS_IOS_CONTENT_SUGGESTIONS_REMOVE)
                              action:^{
                                [weakCommandHandler removeMostVisited:weakItem];
                              }
                               style:UIAlertActionStyleDestructive];

  [alertCoordinator addItemWithTitle:l10n_util::GetNSString(IDS_APP_CANCEL)
                              action:nil
                               style:UIAlertActionStyleCancel];

  return alertCoordinator;
}

@end
