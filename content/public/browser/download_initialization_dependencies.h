// Copyright (c) 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_PUBLIC_BROWSER_DOWNLOAD_INITIALIZATION_DEPENDENCIES_H_
#define CONTENT_PUBLIC_BROWSER_DOWNLOAD_INITIALIZATION_DEPENDENCIES_H_

#include <string>

#include "content/common/content_export.h"

namespace content {

enum DownloadInitializationDependency {
  DOWNLOAD_INITIALIZATION_DEPENDENCY_NONE = 0,

#define INITIALIZATION_DEPENDENCY(name, value) \
  DOWNLOAD_INITIALIZATION_DEPENDENCY_##name = value,

#include "content/public/browser/download_initialization_dependency_values.h"

#undef INITIALIZATION_DEPENDENCY
};

std::string CONTENT_EXPORT DownloadInitializationDependencyToString(
    DownloadInitializationDependency error);

}  // namespace content

#endif  // CONTENT_PUBLIC_BROWSER_DOWNLOAD_INITIALIZATION_DEPENDENCIES_H_
