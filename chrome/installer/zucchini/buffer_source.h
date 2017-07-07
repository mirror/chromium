// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_INSTALLER_ZUCCHINI_BUFFER_SOURCE_H_
#define CHROME_INSTALLER_ZUCCHINI_BUFFER_SOURCE_H_

#include <algorithm>
#include <cstddef>
#include <cstdint>

#include "chrome/installer/zucchini/buffer_view.h"

namespace zucchini {

// BufferSource acts like an input stream with convenience methods to parse data
// from a contiguous sequence of raw data. It keeps a cursor pointing at the
// current  read position.
class BufferSource : public ConstBufferView {
 public:
  static BufferSource FromRange(const_iterator first, const_iterator last) {
    return BufferSource(ConstBufferView::FromRange(first, last));
  }

  using ConstBufferView::ConstBufferView;
  BufferSource() = default;
  explicit BufferSource(ConstBufferView buffer);
  BufferSource(const BufferSource&) = default;

  using ConstBufferView::operator=;
  BufferSource& operator=(const BufferSource&) = default;

  // Moves cursor forward by min(|n|, Remaining()). Returns a reference to self.
  BufferSource& Skip(size_type n);

  // Returns true if |value| can be read by reinterpreting data as type |T|,
  // starting at cursor + |pos|.
  template <class T>
  bool ExpectValue(const T& value, size_type pos = 0) {
    DCHECK(begin() != nullptr);
    if (begin() + pos + sizeof(T) > end())
      return false;
    return value == *reinterpret_cast<const T*>(begin() + pos);
  }

  // Returns true if |bytes| can be read starting at cursor + |pos|.
  bool ExpectBytes(std::initializer_list<uint8_t> bytes,
                   size_type pos = 0) const;

  // Same as ExpectBytes(), but moves the cursor by bytes.size() if read is
  // successfull.
  bool ConsumeBytes(std::initializer_list<uint8_t> bytes);

  // Tries to reinterpret data as type |T|, starting at cursor and to write the
  // result into |value|, moving the cursor forward by sizeof(T). Returns true
  // if sufficient data is available, and false otherwise.
  template <class T>
  bool ReadValue(T* value) {
    DCHECK(begin() != nullptr);
    if (begin() + sizeof(T) > end())
      return false;
    *value = *reinterpret_cast<const T*>(begin());
    remove_prefix(sizeof(T));
    return true;
  }

  // Tries to reinterpret data as type |T| at cursor and to return a
  // reinterpreted pointer of type |T| pointing into the underlying data, moving
  // the cursor forward by sizeof(T). Returns nullptr if insufficient data is
  // available.
  template <class T>
  const T* GetPointer() {
    DCHECK(begin() != nullptr);
    if (begin() + sizeof(T) > end())
      return nullptr;
    const T* ptr = reinterpret_cast<const T*>(begin());
    remove_prefix(sizeof(T));
    return ptr;
  }

  // Tries to reinterpret data as an array of type |T| with |count| elements,
  // starting at cursor, and to return a reinterpreted pointer of type |T|
  // pointing into the underlying data, moving the cursor forward by sizeof(T).
  // Returns nullptr if insufficient data is available.
  template <class T>
  const T* GetArray(size_t count) {
    if (Remaining() < count * sizeof(T))
      return nullptr;
    const T* array = reinterpret_cast<const T*>(begin());
    remove_prefix(count * sizeof(T));
    return array;
  }

  // Returns a BufferView starting at cursor and with size
  // min(|size|, Remaining()).
  ConstBufferView GetRegion(size_type size);

  // Returns the number of bytes remaining from cursor until end.
  size_t Remaining() const;
};

}  // namespace zucchini

#endif  // CHROME_INSTALLER_ZUCCHINI_BUFFER_SOURCE_H_
