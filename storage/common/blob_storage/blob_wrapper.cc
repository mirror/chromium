// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "storage/common/blob_storage/blob_wrapper.h"

namespace storage {

BlobWrapper::BlobWrapper(mojom::BlobPtr blob) : blob_(std::move(blob)) {}

BlobWrapper::BlobWrapper(const BlobWrapper& other) {
  if (other.blob_)
    other.blob_->Clone(MakeRequest(&blob_));
}

BlobWrapper::BlobWrapper(BlobWrapper&& other) = default;

BlobWrapper& BlobWrapper::operator=(const BlobWrapper& other) {
  if (other.blob_)
    other.blob_->Clone(MakeRequest(&blob_));
  return *this;
}

BlobWrapper& BlobWrapper::operator=(BlobWrapper&& other) = default;

BlobWrapper::~BlobWrapper() = default;

bool BlobWrapper::is_bound() const {
  return blob_.is_bound();
}

mojom::BlobPtr BlobWrapper::ExtractPtr() {
  return std::move(blob_);
}

}  // namespace storage
