// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_OFFLINE_PAGES_CORE_PREFETCH_STORE_PREFETCH_DOWNLOADER_QUOTA_H_
#define COMPONENTS_OFFLINE_PAGES_CORE_PREFETCH_STORE_PREFETCH_DOWNLOADER_QUOTA_H_

#include <cstdint>

#include "base/macros.h"

namespace sql {
class Connection;
}  // namespace sql

namespace offline_pages {

class PrefetchDownloaderQuota {
 public:
  static int64_t GetAvailableQuota(sql::Connection* db);
  static bool SetAvailableQuota(sql::Connection* db, int64_t quota);

 private:
  DISALLOW_COPY_AND_ASSIGN(PrefetchDownloaderQuota);
};
}  // namespace offline_pages

#endif  // COMPONENTS_OFFLINE_PAGES_CORE_PREFETCH_STORE_PREFETCH_DOWNLOADER_QUOTA_H_
