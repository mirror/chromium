// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/ui/bookmarks/bookmark_mediator.h"

#include "base/metrics/user_metrics.h"
#include "base/metrics/user_metrics_action.h"
#include "base/strings/sys_string_conversions.h"
#include "components/bookmarks/browser/bookmark_model.h"
#include "components/bookmarks/browser/bookmark_node.h"
#include "components/bookmarks/browser/bookmark_utils.h"
#include "components/pref_registry/pref_registry_syncable.h"
#include "components/prefs/pref_service.h"
#include "ios/chrome/browser/bookmarks/bookmark_model_factory.h"
#include "ios/chrome/browser/browser_state/chrome_browser_state.h"
#include "ios/chrome/browser/pref_names.h"
#import "ios/chrome/browser/ui/bookmarks/bookmark_edit_view_controller.h"
#import "ios/chrome/browser/ui/bookmarks/bookmark_home_view_controller.h"
#import "ios/chrome/browser/ui/bookmarks/bookmark_utils_ios.h"
#import "ios/chrome/browser/ui/commands/snackbar_action.h"
#import "ios/chrome/browser/ui/commands/snackbar_commands.h"
#include "ios/chrome/browser/ui/uikit_ui_util.h"
#include "ios/chrome/grit/ios_strings.h"
#include "ui/base/l10n/l10n_util.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

using bookmarks::BookmarkModel;
using bookmarks::BookmarkNode;

namespace {
const int64_t kLastUsedFolderNone = -1;
}  // namespace

@interface BookmarkMediator ()

// BrowserState for this mediator.
@property(nonatomic, assign) ios::ChromeBrowserState* browserState;

// Dispatcher for sending commands.
@property(nonatomic, weak, readonly) id<SnackbarCommands> dispatcher;

@end

@implementation BookmarkMediator

@synthesize browserState = _browserState;
@synthesize dispatcher = _dispatcher;

+ (void)registerBrowserStatePrefs:(user_prefs::PrefRegistrySyncable*)registry {
  registry->RegisterInt64Pref(prefs::kIosBookmarkFolderDefault,
                              kLastUsedFolderNone);
}

+ (const BookmarkNode*)folderForNewBookmarksInBrowserState:
    (ios::ChromeBrowserState*)browserState {
  bookmarks::BookmarkModel* bookmarks =
      ios::BookmarkModelFactory::GetForBrowserState(browserState);
  const BookmarkNode* defaultFolder = bookmarks->mobile_node();

  PrefService* prefs = browserState->GetPrefs();
  int64_t node_id = prefs->GetInt64(prefs::kIosBookmarkFolderDefault);
  if (node_id == kLastUsedFolderNone)
    node_id = defaultFolder->id();
  const BookmarkNode* result =
      bookmarks::GetBookmarkNodeByID(bookmarks, node_id);

  if (result)
    return result;

  return defaultFolder;
}

+ (void)setFolderForNewBookmarks:(const BookmarkNode*)folder
                  inBrowserState:(ios::ChromeBrowserState*)browserState {
  DCHECK(folder && folder->is_folder());
  browserState->GetPrefs()->SetInt64(prefs::kIosBookmarkFolderDefault,
                                     folder->id());
}

- (instancetype)initWithBrowserState:(ios::ChromeBrowserState*)browserState
                          dispatcher:(id<SnackbarCommands>)dispatcher {
  self = [super init];
  if (self) {
    _browserState = browserState;
    _dispatcher = dispatcher;
  }
  return self;
}

- (void)addBookmarkWithTitle:(NSString*)title
                         URL:(const GURL&)URL
                  editAction:(void (^)())editAction {
  base::RecordAction(base::UserMetricsAction("BookmarkAdded"));
  const BookmarkNode* defaultFolder =
      [[self class] folderForNewBookmarksInBrowserState:self.browserState];
  BookmarkModel* bookmarkModel =
      ios::BookmarkModelFactory::GetForBrowserState(self.browserState);
  bookmarkModel->AddURL(defaultFolder, defaultFolder->child_count(),
                        base::SysNSStringToUTF16(title), URL);
  SnackbarAction* action = [[SnackbarAction alloc] init];
  action.handler = editAction;
  action.title = l10n_util::GetNSString(IDS_IOS_NAVIGATION_BAR_EDIT_BUTTON);
  action.accessibilityIdentifier = @"Edit";

  NSString* folderTitle =
      bookmark_utils_ios::TitleForBookmarkNode(defaultFolder);
  NSString* text =
      self.browserState->GetPrefs()->GetInt64(
          prefs::kIosBookmarkFolderDefault) != kLastUsedFolderNone
          ? l10n_util::GetNSStringF(IDS_IOS_BOOKMARK_PAGE_SAVED_FOLDER,
                                    base::SysNSStringToUTF16(folderTitle))
          : l10n_util::GetNSString(IDS_IOS_BOOKMARK_PAGE_SAVED);
  TriggerHapticFeedbackForNotification(UINotificationFeedbackTypeSuccess);
  [self.dispatcher
      showSnackbarWithMessage:text
                     category:bookmark_utils_ios::kBookmarksSnackbarCategory
                       action:action];
}

@end
