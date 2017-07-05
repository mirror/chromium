// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_INSTALLER_ZUCCHINI_BUFFER_SOURCE_H_
#define CHROME_INSTALLER_ZUCCHINI_BUFFER_SOURCE_H_

#include <cstddef>
#include <cstdint>

#include "base/optional.h"
#include "chrome/installer/zucchini/buffer_view.h"

namespace zucchini {

class BufferSource : public BufferView {
 public:

  static BufferSource FromRange(const_iterator first, const_iterator last) {
    return {BufferView::FromRange(first, last)};
  }

  using BufferView::BufferView;
  using BufferView::operator=;
  BufferSource() = default;
  BufferSource(BufferView buffer);
  BufferSource(const BufferSource&) = default;
  BufferSource& operator=(BufferSource&) = default;

  template <class T>
  bool ExpectValue(const T& value) {
    DCHECK(begin() != nullptr);
    if (begin() + sizeof(T) > end())
      return false;
    return value == *reinterpret_cast<const T*>(begin());
  }

  template <class T>
  bool ReadValue(T* value) {
    DCHECK(begin() != nullptr);
    if (begin() + sizeof(T) > end())
      return false; 
    *value = *reinterpret_cast<const T*>(begin());
    remove_prefix(sizeof(T));
    return true;
  }
  
  template <class T>
  const T* GetValue();

  BufferView GetRegion(size_t size);

  bool ExpectBytes(std::initializer_list<uint8_t> bytes) const;
  bool ConsumeBytes(std::initializer_list<uint8_t> bytes);

  size_t Remaining() const;

};

}  // namespace zucchini

#endif  // CHROME_INSTALLER_ZUCCHINI_BUFFER_SOURCE_H_
