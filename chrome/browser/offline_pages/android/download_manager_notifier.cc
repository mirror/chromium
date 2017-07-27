// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/offline_pages/download_manager_notifier.h"

#include <memory>

#include "base/bind.h"
#include "chrome/browser/android/offline_pages/downloads/offline_page_download_manager_bridge.h"

namespace offline_pages {
namespace {
const char kUserDataKey[] = "offline_page_dl_manager_notifier";

void LoggingCallback(int64_t offline_id, int64_t system_download_id) {
  LOG(ERROR) << "fgorski: OID: " << offline_id
             << " SID: " << system_download_id;
}
}  // namespace

void DownloadManagerNotifier::CreateAndStartObserving(OfflinePageModel* model) {
  std::unique_ptr<DownloadManagerNotifier> notifier(
      new DownloadManagerNotifier);
  model->AddObserver(notifier.get());
  model->SetUserData(&kUserDataKey, std::move(notifier));
  LOG(ERROR) << "fgorski: notifier created and added to the model";
}

void DownloadManagerNotifier::OfflinePageModelLoaded(OfflinePageModel* model) {}

void DownloadManagerNotifier::OfflinePageAdded(OfflinePageModel* model,
                                               const OfflinePageItem& page) {
  LOG(ERROR) << "fgorski: added a page.";
  android::OfflinePageDownloadManagerBridge::AddPageToDownloadManager(
      page, base::BindOnce(&LoggingCallback));
}

void DownloadManagerNotifier::OfflinePageDeleted(
    const OfflinePageModel::DeletedPageInfo& page) {}
}  // namespace offline_pages
