// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/media/storage_id.h"

#include "base/callback.h"

// static
void StorageId::ComputeStorageId(const std::string& salt,
                                 const url::Origin& origin,
                                 StorageIdCallback callback) {
  // Not implemented by default.
  std::move(callback).Run(std::string());
}
