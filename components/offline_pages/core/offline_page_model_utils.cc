// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/offline_pages/core/offline_page_model_utils.h"

#include <limits>

#include "base/rand_util.h"

namespace offline_pages {

namespace model_utils {

AddPageResult ItemActionStatusToAddPageResult(ItemActionStatus status) {
  switch (status) {
    case ItemActionStatus::SUCCESS:
      return AddPageResult::SUCCESS;
    case ItemActionStatus::ALREADY_EXISTS:
      return AddPageResult::ALREADY_EXISTS;
    default:
      NOTREACHED();
      return AddPageResult::STORE_FAILURE;
  }
}

int64_t GenerateOfflineId() {
  return base::RandGenerator(std::numeric_limits<int64_t>::max()) + 1;
}

}  // namespace model_utils

}  // namespace offline_pages
