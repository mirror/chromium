// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_INSTALLER_ZUCCHINI_BUFFER_SINK_H_
#define CHROME_INSTALLER_ZUCCHINI_BUFFER_SINK_H_

#include <cassert>

#include "chrome/installer/zucchini/buffer_view.h"
#include "base/logging.h"

namespace zucchini {

class BufferSink : public MutableBufferView {
 public:
  using iterator = MutableBufferView::iterator;

  static BufferSink FromRange(iterator first, iterator last) {
    return {MutableBufferView::FromRange(first, last)};
  }

  using MutableBufferView::MutableBufferView;
  using MutableBufferView::operator=;
  BufferSink(MutableBufferView buffer);
  BufferSink(BufferSink&) = default;
  BufferSink& operator=(BufferSink&) = default;

  template <class T>
  bool PutValue(T value) {
    DCHECK(begin() != nullptr);
    if (begin() + sizeof(T) > end())
      return false;
    *reinterpret_cast<T*>(begin()) = value;
    remove_prefix(sizeof(T));
    return true;
  }

  template <class It>
  bool PutRange(It first, It last) {
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
