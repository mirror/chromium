// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_DOWNLOAD_PUBLIC_COMMON_DOWNLOAD_ITEM_DELEGATE_H_
#define COMPONENTS_DOWNLOAD_PUBLIC_COMMON_DOWNLOAD_ITEM_DELEGATE_H_

#include <string>

#include "components/download/public/common/download_export.h"

namespace download {

// Delegate for operations that a DownloadItem can't provide for itself.
// The base implementation of this class does nothing and derived classed may
// add needed methods.
class COMPONENTS_DOWNLOAD_EXPORT DownloadItemDelegate {
 public:
  virtual ~DownloadItemDelegate() = default;
};

}  // namespace download

#endif  // COMPONENTS_DOWNLOAD_PUBLIC_COMMON_DOWNLOAD_ITEM_DELEGATE_H_
