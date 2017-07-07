// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_INSTALLER_ZUCCHINI_BUFFER_SINK_H_
#define CHROME_INSTALLER_ZUCCHINI_BUFFER_SINK_H_

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <iterator>

#include "base/logging.h"
#include "chrome/installer/zucchini/buffer_view.h"

namespace zucchini {

// BufferSink acts like an output stream with convenience methods to serialize
// data into a contiguous sequence of raw data. It keeps a cursor pointing at
// the current write position.
class BufferSink : public MutableBufferView {
 public:
  using iterator = MutableBufferView::iterator;

  static BufferSink FromRange(iterator first, iterator last) {
    return BufferSink(MutableBufferView::FromRange(first, last));
  }

  using MutableBufferView::MutableBufferView;
  BufferSink() = default;
  explicit BufferSink(MutableBufferView buffer);
  BufferSink(const BufferSink&) = default;

  using MutableBufferView::operator=;
  BufferSink& operator=(const BufferSink&) = default;

  // Tries to write the binary representation of |value| starting at cursor,
  // moving cursor forward by sizeof(T). Returns true if sufficient space is
  // available, and false otherwise.
  template <class T>
  bool PutValue(const T& value) {
    DCHECK(begin() != nullptr);
    if (begin() + sizeof(T) > end())
      return false;
    *reinterpret_cast<T*>(begin()) = value;
    remove_prefix(sizeof(T));
    return true;
  }

  // Tries to write the range [|first|, |last|) at cursor, moving cursor forward
  // by distance(first, last). Returns true if sufficient space is available,
  // and false otherwise.
  template <class It>
  bool PutRange(It first, It last) {
    static_assert(sizeof(typename std::iterator_traits<It>::value_type) ==
                      sizeof(value_type),
                  "value_type should fit in uint8_t");
    DCHECK(begin() != nullptr);
    if (begin() + (last - first) > end())
      return false;
    std::copy(first, last, begin());
    remove_prefix(last - first);
    return true;
  }

  size_t Remaining() const;
};

}  // namespace zucchini

#endif  // CHROME_INSTALLER_ZUCCHINI_BUFFER_SINK_H_
