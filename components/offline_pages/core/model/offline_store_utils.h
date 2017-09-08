// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_OFFLINE_PAGES_CORE_MODEL_OFFLINE_STORE_UTILS_H_
#define COMPONENTS_OFFLINE_PAGES_CORE_MODEL_OFFLINE_STORE_UTILS_H_

#include <stdint.h>

namespace offline_pages {

class OfflineStoreUtils {
 public:
  // Generates a random offline id;
  static int64_t GenerateOfflineId();
};

}  // namespace offline_pages

#endif  // COMPONENTS_OFFLINE_PAGES_CORE_MODEL_OFFLINE_STORE_UTILS_H_
