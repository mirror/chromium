// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_OFFLINE_PAGES_CORE_OFFLINE_PAGE_STORE_UTILS_H_
#define COMPONENTS_OFFLINE_PAGES_CORE_OFFLINE_PAGE_STORE_UTILS_H_

#include <stdint.h>

#include <string>

namespace base {
class Time;
class FilePath;
}  // namespace base

namespace offline_pages {

namespace OfflinePageStoreUtils {

// Time conversion methods for the time format recommended for offline pages
// projects for database storage: elapsed time in microseconds since the Windows
// epoch.
int64_t ToDatabaseTime(base::Time time);
base::Time FromDatabaseTime(int64_t serialized_time);

// File path conversion methods for the file path format recommended for offline
// pages projects for database storage: UTF8 string.
std::string ToDatabaseFilePath(const base::FilePath& file_path);
base::FilePath FromDatabaseFilePath(const std::string& file_path_string);

}  // namespace OfflinePageStoreUtils

}  // namespace offline_pages

#endif  // COMPONENTS_OFFLINE_PAGES_CORE_OFFLINE_PAGE_STORE_UTILS_H_
