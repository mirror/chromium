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

// BufferSource acts like a cursor with convenience methods to parse data from
// a contiguous sequence of raw data.
class BufferSource : public BufferView {
 public:
  static BufferSource FromRange(const_iterator first, const_iterator last) {
    return BufferSource(BufferView::FromRange(first, last));
  }

  using BufferView::BufferView;
  BufferSource() = default;
  explicit BufferSource(BufferView buffer);
  BufferSource(const BufferSource&) = default;

  using BufferView::operator=;
  BufferSource& operator=(const BufferSource&) = default;

  // Returns true if |value| can be read by reinterpreting data at the current
  // cursor as type |T|.
  template <class T>
  bool ExpectValue(const T& value) {
    DCHECK(begin() != nullptr);
    if (begin() + sizeof(T) > end())
      return false;
    return value == *reinterpret_cast<const T*>(begin());
  }

  // Returns true if |bytes| can be read starting at the cursor.
  bool ExpectBytes(std::initializer_list<uint8_t> bytes) const;

  // Same as ExpectBytes(), but moves the curosr by bytes.size().
  bool ConsumeBytes(std::initializer_list<uint8_t> bytes);

  // Tries to reinterpret data at the current cursor as type |T| and write the
  // result into |T|, moving the cursor forward by sizeof(T). Returns true if
  // sufficient data is available to complete the operation, and false
  // otherwise.
  template <class T>
  bool ReadValue(T* value) {
    DCHECK(begin() != nullptr);
    if (begin() + sizeof(T) > end())
      return false;
    *value = *reinterpret_cast<const T*>(begin());
    remove_prefix(sizeof(T));
    return true;
  }

  // Tries to reinterpret data at the current cursor as type |T| and return a
  // reinterpreted pointer of type |T| pointing into the underlying data, moving
  // the cursor forward by sizeof(T). Returns nullptr if insufficient data is
  // available.
  template <class T>
  const T* GetValue() {
    DCHECK(begin() != nullptr);
    if (begin() + sizeof(T) > end())
      return nullptr;
    T* value = *reinterpret_cast<const T*>(begin());
    remove_prefix(sizeof(T));
    return value;
  }

  // Returns a BufferView starting at the current cursor and with size equal to
  // max(|size|, Remaining()).
  BufferView GetRegion(size_t size);

  // Returns the number of bytes remaining from the current cursor until end.
  size_t Remaining() const;
};

}  // namespace zucchini

#endif  // CHROME_INSTALLER_ZUCCHINI_BUFFER_SOURCE_H_
