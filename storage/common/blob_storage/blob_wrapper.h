// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef STORAGE_COMMON_BLOB_STORAGE_BLOB_WRAPPER_H_
#define STORAGE_COMMON_BLOB_STORAGE_BLOB_WRAPPER_H_

#include "storage/common/storage_common_export.h"
#include "storage/public/interfaces/blobs.mojom.h"

namespace storage {

// Copyable wrapper around a mojom::BlobPtr.
class STORAGE_COMMON_EXPORT BlobWrapper {
 public:
  BlobWrapper(mojom::BlobPtr blob = nullptr);
  BlobWrapper(const BlobWrapper& other);
  BlobWrapper(BlobWrapper&& other);
  BlobWrapper& operator=(const BlobWrapper& other);
  BlobWrapper& operator=(BlobWrapper&& other);
  ~BlobWrapper();

  bool is_bound() const;
  mojom::BlobPtr ExtractPtr();

 private:
  mojom::BlobPtr blob_;
};

}  // namespace storage

#endif  // STORAGE_COMMON_BLOB_STORAGE_BLOB_WRAPPER_H_
