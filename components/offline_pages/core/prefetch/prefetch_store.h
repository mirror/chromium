// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_OFFLINE_PAGES_CORE_PREFETCH_PREFETCH_STORE_H_
#define COMPONENTS_OFFLINE_PAGES_CORE_PREFETCH_PREFETCH_STORE_H_

#include <vector>

#include "components/offline_pages/core/prefetch/prefetch_types.h"

namespace offline_pages {

// Persistent storage access class for prefetching offline pages data.
class PrefetchStore {
 public:
  virtual ~PrefetchStore() = default;
  virtual void AddUniqueUrls(const std::vector<PrefetchURL>& prefetch_urls) = 0;
};

}  // namespace offline_pages

#endif  // COMPONENTS_OFFLINE_PAGES_CORE_PREFETCH_PREFETCH_STORE_H_
