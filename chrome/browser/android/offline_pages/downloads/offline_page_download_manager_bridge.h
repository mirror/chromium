// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_ANDROID_OFFLINE_PAGES_DOWNLOADS_OFFLINE_PAGE_DOWNLOAD_MANAGER_BRIDGE_H_
#define CHROME_BROWSER_ANDROID_OFFLINE_PAGES_DOWNLOADS_OFFLINE_PAGE_DOWNLOAD_MANAGER_BRIDGE_H_

#include <stdint.h>

#include "base/callback.h"
#include "base/macros.h"

namespace offline_pages {

struct OfflinePageItem;

namespace android {

// Bridge for interacting with Android Download Manager.
class OfflinePageDownloadManagerBridge {
 public:
  using AddPageCallback = base::OnceCallback<void(int64_t /* offline_id */,
                                                  int64_t /* system_id */)>;

  // Adds an |offline_page| to Android Download Manager's database. Returns a
  // system download ID of the entry through a |callback|.
  static void AddPageToDownloadManager(const OfflinePageItem& offline_page,
                                       AddPageCallback callback);

  DISALLOW_COPY_AND_ASSIGN(OfflinePageDownloadManagerBridge);
};

}  // namespace android
}  // namespace offline_pages

#endif  // CHROME_BROWSER_ANDROID_OFFLINE_PAGES_DOWNLOADS_OFFLINE_PAGE_DOWNLOAD_MANAGER_BRIDGE_H_
