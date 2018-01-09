// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef EXTENSIONS_COMMON_EXTENSION_ICON_PATH_NORMALIZER_H_
#define EXTENSIONS_COMMON_EXTENSION_ICON_PATH_NORMALIZER_H_

#include <set>
#include <string>

#include "base/files/file_path.h"

// Normalize extension icon relative path. Removes ".". Returns false if path
// can not be normalized, i.e. it refer parent or  empty after normalization.
bool NormalizeExtensionIconPath(const base::FilePath& path,
                                base::FilePath* result);

// Applies |NormalizeExtensionIconPath| for each icon path and add it to result
// in the case of success.
std::set<base::FilePath> NormalizeExtensionIconPaths(
    const std::set<base::FilePath>& icons_paths);

#endif  // EXTENSIONS_COMMON_EXTENSION_ICON_PATH_NORMALIZER_H_
