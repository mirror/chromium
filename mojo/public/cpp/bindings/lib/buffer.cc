// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "mojo/public/cpp/bindings/lib/buffer.h"

#include "base/logging.h"
#include "mojo/public/cpp/bindings/lib/bindings_internal.h"

namespace mojo {
namespace internal {

Buffer::Buffer() = default;

Buffer::Buffer(void* data, size_t size)
    : data_(data),
      size_(size),
      cursor_(reinterpret_cast<uintptr_t>(data)),
      data_end_(cursor_ + size_) {
  DCHECK(IsAligned(data_));
}

Buffer::Buffer(Buffer&& other) {
  *this = std::move(other);
}

Buffer::~Buffer() = default;

Buffer& Buffer::operator=(Buffer&& other) {
  data_ = other.data_;
  size_ = other.size_;
  cursor_ = other.cursor_;
  data_end_ = other.data_end_;
  other.Reset();
  return *this;
}

void* Buffer::Allocate(size_t num_bytes) {
  num_bytes = Align(num_bytes);
  uintptr_t result = cursor_;
  cursor_ += num_bytes;
  if (cursor_ > data_end_ || cursor_ < result) {
    NOTREACHED();
    cursor_ -= num_bytes;
    return nullptr;
  }

  return reinterpret_cast<void*>(result);
}

void Buffer::Reset() {
  data_ = nullptr;
  size_ = 0;
  cursor_ = 0;
  data_end_ = 0;
}

}  // namespace internal
}  // namespace mojo
