// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_OFFLINE_PAGES_ANDROID_OFFLINE_PAGES_DOWNLOAD_MANAGER_BRIDGE_H_
#define CHROME_BROWSER_OFFLINE_PAGES_ANDROID_OFFLINE_PAGES_DOWNLOAD_MANAGER_BRIDGE_H_

#include <vector>

namespace offline_pages {
namespace android {

// Bridge between C++ and Java for implementing background scheduler
// on Android.
class OfflinePagesDownloadManagerBridge {
 public:
  // Returns true if ADM is available on this phone.
  static bool IsAndroidDownloadManagerInstalled();

  // Returns the ADM ID of the download, which we will place in the offline
  // pages database as part of the offline page item.
  static long addCompletedDownload(const std::string& title,
                                   const std::string& description,
                                   const std::string& path,
                                   long length,
                                   const std::string& uri,
                                   const std::string& referer);

  // Returns the number of pages removed.
  static long remove(const std::vector<int64_t>& android_download_manager_ids);
};

}  // namespace android
}  // namespace offline_pages

#endif  // CHROME_BROWSER_OFFLINE_PAGES_ANDROID_OFFLINE_PAGES_DOWNLOAD_MANAGER_BRIDGE_H_
