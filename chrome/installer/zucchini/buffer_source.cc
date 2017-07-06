// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/installer/zucchini/buffer_source.h"

namespace zucchini {

BufferSource::BufferSource(ConstBufferView buffer) : ConstBufferView(buffer) {}

BufferSource& Skip(size_type n) {
  remove_prefix(std::min(n, Remaining()));
  return *this;
}

ConstBufferView BufferSource::GetRegion(size_t count) {
  DCHECK(begin() != nullptr);
  if (begin() + count > end()) {
    ConstBufferView buffer = ConstBufferView::FromRange(begin(), end());
    remove_prefix(size());
    return buffer;
  }
  ConstBufferView buffer(begin(), count);
  remove_prefix(count);
  return buffer;
}

bool BufferSource::ExpectBytes(std::initializer_list<uint8_t> bytes,
                               size_type pos) const {
  if (begin() + pos + bytes.size() > end())
    return false;
  const_iterator it = begin() + pos;
  for (auto byte : bytes) {
    if (byte != *it++)
      return false;
  }
  return true;
}

bool BufferSource::ConsumeBytes(std::initializer_list<uint8_t> bytes) {
  if (!ExpectBytes(bytes)) {
    return false;
  }
  remove_prefix(bytes.size());
  return true;
}

size_t BufferSource::Remaining() const {
  return size();
}

}  // namespace zucchini
