// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/ui/external_file_remover.h"

#include "base/logging.h"
#import "base/mac/bind_objc_block.h"
#include "base/strings/sys_string_conversions.h"
#include "base/task_scheduler/post_task.h"
#include "components/bookmarks/browser/bookmark_model.h"
#include "components/sessions/core/tab_restore_service.h"
#include "ios/chrome/browser/bookmarks/bookmark_model_factory.h"
#include "ios/chrome/browser/browser_state/chrome_browser_state.h"
#include "ios/chrome/browser/chrome_url_util.h"
#include "ios/chrome/browser/sessions/ios_chrome_tab_restore_service_factory.h"
#import "ios/chrome/browser/tabs/tab_model.h"
#import "ios/chrome/browser/tabs/tab_model_list.h"
#import "ios/chrome/browser/ui/external_file_controller.h"
#include "ios/web/public/web_thread.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

ExternalFileRemover::ExternalFileRemover(ios::ChromeBrowserState* browserState)
    : bookmarkModel_(
          ios::BookmarkModelFactory::GetForBrowserState(browserState)),
      tabRestoreService_(NULL),
      browserState_(browserState),
      weak_ptr_factory_(this) {}

ExternalFileRemover::~ExternalFileRemover() {
  if (tabRestoreService_)
    tabRestoreService_->RemoveObserver(this);
}

void ExternalFileRemover::TabRestoreServiceChanged(
    sessions::TabRestoreService* service) {
  if (service->IsLoaded()) {
    tabRestoreService_->RemoveObserver(this);
    RemoveFiles(false, base::Closure());
  }
}

void ExternalFileRemover::TabRestoreServiceDestroyed(
    sessions::TabRestoreService* service) {
  // This happens during shutdown and RemoveFiles() cannot be safely called,
  // so it is a no-op.
}

void ExternalFileRemover::Remove(bool all_files,
                                 const base::Closure& callback) {
  // |IOSChromeTabRestoreServiceFactory::GetForBrowserState| has to be called on
  // the UI thread.
  DCHECK_CURRENTLY_ON(web::WebThread::UI);
  tabRestoreService_ =
      IOSChromeTabRestoreServiceFactory::GetForBrowserState(browserState_);
  DCHECK(tabRestoreService_);
  if (!tabRestoreService_->IsLoaded()) {
    // TODO(crbug.com/430902): In the case of the presence of tab restore
    // service, only unreferenced files are removed. This can be addressed with
    // the larger problem of Clear All browsing data not clearing Tab Restore.
    tabRestoreService_->AddObserver(this);
    tabRestoreService_->LoadTabsFromLastSession();
    if (!callback.is_null()) {
      web::WebThread::PostTask(web::WebThread::UI, FROM_HERE, callback);
    }
  } else {
    RemoveFiles(all_files, callback);
  }
}

void ExternalFileRemover::RemoveFiles(bool all_files,
                                      const base::Closure& callback) {
  // Add files from current TabModel.
  NSSet* tabModelFiles;
  NSArray<TabModel*>* tabModels =
      GetTabModelsForChromeBrowserState(browserState_);
  for (TabModel* tabModel in tabModels) {
    if (tabModel.currentTab) {
      tabModelFiles = [tabModel currentlyReferencedExternalFiles];
    }
  }

  // Add Files from Bookmarks.
  std::vector<bookmarks::BookmarkModel::URLAndTitle> bookmarks;
  bookmarkModel_->GetBookmarks(&bookmarks);
  NSMutableSet* bookmarkedFiles = [NSMutableSet set];
  for (const auto& bookmark : bookmarks) {
    GURL bookmarkUrl = bookmark.url;
    if (UrlIsExternalFileReference(bookmarkUrl)) {
      [bookmarkedFiles
          addObject:base::SysUTF8ToNSString(bookmarkUrl.ExtractFileName())];
    }
  }
  NSSet* allFiles = [tabModelFiles setByAddingObjectsFromSet:bookmarkedFiles];

  NSSet* referencedFiles = all_files ? nil : allFiles;
  const NSInteger kMinimumAgeInDays = 30;
  NSInteger ageInDays = all_files ? 0 : kMinimumAgeInDays;

  base::Closure callback_wrapper = callback;
  if (callback_wrapper.is_null()) {
    callback_wrapper = base::Bind(&base::DoNothing);
  }
  base::PostTaskWithTraitsAndReply(
      FROM_HERE, {base::MayBlock(), base::TaskPriority::BACKGROUND},
      base::BindBlockArc(^{
        [ExternalFileController removeFilesExcluding:referencedFiles
                                           olderThan:ageInDays];
      }),
      callback_wrapper);
}

void ExternalFileRemover::RemoveAfterDelay(const base::TimeDelta& delay,
                                           const base::Closure& callback) {
  bool remove_all_files = delay == base::TimeDelta::FromSeconds(0);
  // Creating a copy so it can be used from the block underneath.
  base::Closure callback_copy = callback;
  // Since a method on |this| is called from a block, this dance is necessary to
  // make sure a method on |this| is not called when the object has gone away.
  base::WeakPtr<ExternalFileRemover> weak_this = weak_ptr_factory_.GetWeakPtr();
  web::WebThread::PostDelayedTask(
      web::WebThread::UI, FROM_HERE, base::BindBlockArc(^{
        if (weak_this) {
          weak_this->Remove(remove_all_files, callback_copy);
        } else if (!callback_copy.is_null()) {
          callback_copy.Run();
        }
      }),
      delay);
}
