// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/ui/bookmarks/bookmark_path_cache.h"

#include "components/bookmarks/browser/bookmark_model.h"
#include "components/bookmarks/browser/bookmark_node.h"
#include "components/pref_registry/pref_registry_syncable.h"
#include "components/prefs/pref_service.h"
#include "ios/chrome/browser/bookmarks/bookmark_model_factory.h"
#include "ios/chrome/browser/browser_state/chrome_browser_state.h"
#include "ios/chrome/browser/pref_names.h"
#import "ios/chrome/browser/ui/bookmarks/bookmark_edit_view_controller.h"
#import "ios/chrome/browser/ui/bookmarks/bookmark_home_view_controller.h"
#import "ios/chrome/browser/ui/bookmarks/bookmark_utils_ios.h"
#include "ios/chrome/browser/ui/uikit_ui_util.h"
#include "ui/base/l10n/l10n_util.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

using bookmarks::BookmarkModel;
using bookmarks::BookmarkNode;

namespace {
const int64_t kFolderNone = -1;
}  // namespace

@implementation BookmarkPathCache
// Registers the feature preferences.
+ (void)registerBrowserStatePrefs:(user_prefs::PrefRegistrySyncable*)registry {
  registry->RegisterInt64Pref(prefs::kIosBookmarkCachedFolderId, kFolderNone);
  registry->RegisterDoublePref(prefs::kIosBookmarkCachedScrollPosition, 0);
}

// Caches the bookmark UI position that the user was last viewing.
+ (void)cacheBookmarkUIPositionWithPrefService:(PrefService*)prefService
                                      folderId:(int64_t)folderId
                                scrollPosition:(double)scrollPosition {
  prefService->SetInt64(prefs::kIosBookmarkCachedFolderId, folderId);
  prefService->SetDouble(prefs::kIosBookmarkCachedScrollPosition,
                         scrollPosition);
}

// Gets the bookmark UI position that the user was last viewing. Returns YES if
// a valid cache exists. |folderId| and |scrollPosition| are out variables, only
// populated if the return is YES.
+ (BOOL)getBookmarkUIPositionCacheWithPrefService:(PrefService*)prefService
                                            model:
                                                (bookmarks::BookmarkModel*)model
                                         folderId:(int64_t*)folderId
                                   scrollPosition:(double*)scrollPosition {
  *folderId = prefService->GetInt64(prefs::kIosBookmarkCachedFolderId);

  // If the cache was at root node, consider it as nothing was cached.
  if (*folderId == kFolderNone || *folderId == model->root_node()->id())
    return NO;

  // Create bookmark Path.
  const BookmarkNode* bookmark =
      bookmark_utils_ios::FindFolderById(model, *folderId);
  // The bookmark node is gone from model, maybe deleted remotely.
  if (!bookmark)
    return NO;

  *scrollPosition =
      prefService->GetDouble(prefs::kIosBookmarkCachedScrollPosition);
  return YES;
}

// Clears the bookmark UI position cache.
+ (void)clearBookmarkUIPositionCacheWithPrefService:(PrefService*)prefService {
  prefService->SetInt64(prefs::kIosBookmarkCachedFolderId, kFolderNone);
}

@end
