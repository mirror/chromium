// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/offline_pages/core/offline_page_store_utils.h"

#include <string>

#include "base/files/file_util.h"
#include "base/time/time.h"

namespace offline_pages {

namespace store_utils {

int64_t ToDatabaseTime(base::Time time) {
  return time.since_origin().InMicroseconds();
}

base::Time FromDatabaseTime(int64_t serialized_time) {
  return base::Time() + base::TimeDelta::FromMicroseconds(serialized_time);
}

std::string ToDatabaseFilePath(const base::FilePath& file_path) {
  return file_path.AsUTF8Unsafe();
}

base::FilePath FromDatabaseFilePath(const std::string& file_path_string) {
  return base::FilePath::FromUTF8Unsafe(file_path_string);
}

}  // namespace store_utils

}  // namespace offline_pages
