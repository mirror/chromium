// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/offline_pages/core/model/offline_store_utils.h"

#include "base/files/file_path.h"
#include "base/strings/utf_string_conversions.h"
#include "build/build_config.h"

namespace offline_pages {

// static
std::string OfflineStoreUtils::GetUTF8StringFromPath(
    const base::FilePath& path) {
#if defined(OS_POSIX)
  return path.value();
#elif defined(OS_WIN)
  return base::WideToUTF8(path.value());
#else
#error Unknown OS
#endif
}

// static
base::FilePath OfflineStoreUtils::GetPathFromUTF8String(
    const std::string& path_string) {
#if defined(OS_POSIX)
  return base::FilePath(path_string);
#elif defined(OS_WIN)
  return base::FilePath(base::UTF8ToWide(path_string));
#else
#error Unknown OS
#endif
}

}  // namespace offline_pages
