// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/installer/zucchini/buffer_source.h"

namespace zucchini {

BufferSource::BufferSource(ConstBufferView buffer) : ConstBufferView(buffer) {}

BufferSource& BufferSource::Skip(size_type n) {
  remove_prefix(std::min(n, Remaining()));
  return *this;
}

bool BufferSource::CheckNextBytes(std::initializer_list<uint8_t> bytes) const {
  if (Remaining() < bytes.size())
    return false;
  return std::mismatch(bytes.begin(), bytes.end(), begin()).first ==
         bytes.end();
}

bool BufferSource::ConsumeBytes(std::initializer_list<uint8_t> bytes) {
  if (!CheckNextBytes(bytes)) {
    return false;
  }
  remove_prefix(bytes.size());
  return true;
}

ConstBufferView BufferSource::GetRegion(size_t count) {
  DCHECK_NE(begin(), nullptr);
  if (Remaining() < count) {
    ConstBufferView buffer = ConstBufferView::FromRange(begin(), end());
    remove_prefix(size());
    return buffer;
  }
  ConstBufferView buffer(begin(), count);
  remove_prefix(count);
  return buffer;
}

BufferSource::size_type BufferSource::Remaining() const {
  return size();
}

}  // namespace zucchini
