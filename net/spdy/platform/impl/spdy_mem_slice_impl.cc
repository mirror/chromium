// Copyright (c) 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "net/spdy/platform/impl/spdy_mem_slice_impl.h"

#include <utility>

#include "base/logging.h"

namespace net {

SpdyMemSliceImpl::SpdyMemSliceImpl() = default;

SpdyMemSliceImpl::SpdyMemSliceImpl(const char* data, const int length) {
  io_buffer_ = new IOBufferWithSize(length);
  memcpy(io_buffer_->data(), data, length);
}

SpdyMemSliceImpl::SpdyMemSliceImpl(SpdyMemSliceImpl&& other)
    : io_buffer_(std::move(other.io_buffer_)) {
  other.io_buffer_ = nullptr;
}

SpdyMemSliceImpl& SpdyMemSliceImpl::operator=(SpdyMemSliceImpl&& other) {
  io_buffer_ = std::move(other.io_buffer_);
  other.io_buffer_ = nullptr;
  return *this;
}

SpdyMemSliceImpl::~SpdyMemSliceImpl() = default;

const char* SpdyMemSliceImpl::data() const {
  return io_buffer_ != nullptr ? io_buffer_->data() : nullptr;
}

size_t SpdyMemSliceImpl::length() const {
  return io_buffer_ != nullptr ? io_buffer_->size() : 0;
}

}  // namespace net
