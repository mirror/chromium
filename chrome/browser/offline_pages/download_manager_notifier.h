// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_OFFLINE_PAGES_DOWNLOAD_MANAGER_NOTIFIER_H_
#define CHROME_BROWSER_OFFLINE_PAGES_DOWNLOAD_MANAGER_NOTIFIER_H_

#include "base/supports_user_data.h"
#include "components/offline_pages/core/offline_page_model.h"

namespace offline_pages {

// Class responsible for notifying OS download manager of newly added offline
// pages.
class DownloadManagerNotifier : public OfflinePageModel::Observer,
                                public base::SupportsUserData::Data {
 public:
  DownloadManagerNotifier() = default;
  ~DownloadManagerNotifier() override = default;

  static void CreateAndStartObserving(OfflinePageModel* model);

  void OfflinePageModelLoaded(OfflinePageModel* model) override;

  void OfflinePageAdded(OfflinePageModel* model,
                        const OfflinePageItem& added_page) override;

  void OfflinePageDeleted(
      const OfflinePageModel::DeletedPageInfo& page_info) override;

  DISALLOW_COPY_AND_ASSIGN(DownloadManagerNotifier);
};

}  // namespace offline_pages

#endif  // CHROME_BROWSER_OFFLINE_PAGES_DOWNLOAD_MANAGER_NOTIFIER_H_
