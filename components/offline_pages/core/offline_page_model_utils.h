// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_OFFLINE_PAGES_CORE_MODEL____OFFLINE_PAGE_MODEL_UTILS_H_
#define COMPONENTS_OFFLINE_PAGES_CORE_MODEL____OFFLINE_PAGE_MODEL_UTILS_H_

#include <stdint.h>

#include "components/offline_pages/core/offline_page_types.h"
#include "components/offline_pages/core/offline_store_types.h"

namespace offline_pages {

namespace model_utils {

// Converts an ItemActionStatus to AddPageResult.
AddPageResult ItemActionStatusToAddPageResult(ItemActionStatus status);

// Generates a random int64_t, generally used for offline ids.
int64_t GenerateOfflineId();

}  // namespace model_utils

}  // namespace offline_pages

#endif  // COMPONENTS_OFFLINE_PAGES_CORE_MODEL____OFFLINE_PAGE_MODEL_UTILS_H_
