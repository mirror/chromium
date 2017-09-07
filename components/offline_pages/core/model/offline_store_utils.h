// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_OFFLINE_PAGES_CORE_MODEL_OFFLINE_STORE_UTILS_H_
#define COMPONENTS_OFFLINE_PAGES_CORE_MODEL_OFFLINE_STORE_UTILS_H_

#include <string>

namespace base {
class FilePath;
}  // namespace base

namespace offline_pages {

class OfflineStoreUtils {
 public:
  // Converts a file path to a UTF8 string.
  static std::string GetUTF8StringFromPath(const base::FilePath& path);
};

}  // namespace offline_pages

#endif  // COMPONENTS_OFFLINE_PAGES_CORE_MODEL_OFFLINE_STORE_UTILS_H_
