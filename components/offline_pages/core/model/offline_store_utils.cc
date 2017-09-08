// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
#include "components/offline_pages/core/model/offline_store_utils.h"

#include "base/rand_util.h"

namespace offline_pages {

// static
int64_t OfflineStoreUtils::GenerateOfflineId() {
  return base::RandGenerator(std::numeric_limits<int64_t>::max()) + 1;
}

}  // namespace offline_pages
