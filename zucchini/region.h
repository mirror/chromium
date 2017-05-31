// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ZUCCHINI_REGION_H_
#define ZUCCHINI_REGION_H_

#include <cstddef>
#include <cstdint>

namespace zucchini {

class Region {
 public:
  using iterator = uint8_t*;
  using const_iterator = const uint8_t*;

  Region() = default;
  Region(iterator first, size_t length) : first_(first), length_(length) {}
  Region(iterator first, iterator last)
      : first_(first), length_(last - first) {}
  Region(const Region&) = default;
  Region& operator=(const Region&) = default;
  bool operator==(const Region& other) const {
    return first_ == other.first_ && length_ == other.length_;
  }

  template <class T = uint8_t>
  static const T& at(const_iterator it, size_t i = 0) {
    return *reinterpret_cast<const T*>(it + i);
  }
  template <class T = uint8_t>
  static T& at(iterator it, size_t i = 0) {
    return *reinterpret_cast<T*>(it + i);
  }

  template <class T>
  static const T* as(const_iterator it, size_t i = 0) {
    return reinterpret_cast<const T*>(it + i);
  }
  template <class T>
  static T* as(iterator it, size_t i = 0) {
    return reinterpret_cast<T*>(it + i);
  }

  template <class T = uint8_t>
  const T& at(size_t i = 0) const {
    return *reinterpret_cast<const T*>(first_ + i);
  }
  template <class T = uint8_t>
  T& at(size_t i = 0) {
    return *reinterpret_cast<T*>(first_ + i);
  }

  template <class T>
  const T* as(size_t i = 0) const {
    return reinterpret_cast<const T*>(first_ + i);
  }
  template <class T>
  T* as(size_t i = 0) {
    return reinterpret_cast<T*>(first_ + i);
  }

  iterator begin() { return first_; }
  iterator end() { return first_ + length_; }
  const_iterator begin() const { return first_; }
  const_iterator end() const { return first_ + length_; }
  const_iterator cbegin() const { return first_; }
  const_iterator cend() const { return first_ + length_; }

  bool empty() const { return length_ == 0; }
  size_t size() const { return length_; }
  void resize(size_t length) { length_ = length; }

  uint8_t operator[](std::ptrdiff_t n) const { return first_[n]; }
  uint8_t& operator[](std::ptrdiff_t n) { return first_[n]; }

  uint32_t crc32() const;

 private:
  iterator first_ = nullptr;
  size_t length_ = 0;
};

// A wrapper over Region that supplies a "cursor" to sequentially read diverse
// data types. Also adds syntactic sugar to aid parsing binary formats.
class RegionStream {
 public:
  explicit RegionStream(Region region)
      : region_(region), cur_(region_.begin()) {}

  inline size_t Remaining() { return region_.end() - cur_; }

  template <typename T>
  bool FitsArray(size_t count) {
    return Remaining() / sizeof(T) >= count;  // Use divide to prevent overflow.
  }

  template <typename T>
  bool ReadTo(T* v) {
    if (Remaining() < sizeof(T))
      return false;
    *v = Region::at<T>(cur_);
    cur_ += sizeof(T);
    return true;
  }

  template <typename T>
  bool ReadAs(T** ptr) {
    if (Remaining() < sizeof(T))
      return false;
    *ptr = Region::as<T>(cur_);
    cur_ += sizeof(T);
    return true;
  }

  template <typename T>
  bool ReadAsArray(T** ptr, size_t count) {
    if (Remaining() / sizeof(T) < count)
      return false;
    if (count == 0) {
      *ptr = nullptr;
    } else {
      *ptr = Region::as<T>(cur_);
      cur_ += sizeof(T) * count;
    }
    return true;
  }

  bool Seek(size_t offset) {
    if (offset > region_.size())
      return false;
    cur_ = region_.begin() + offset;
    return true;
  }

  // Imposes expectations on next byte(s). Useful for checking magic signatures.
  inline bool Eat(uint8_t ch) {
    if (Remaining() < 1)
      return false;
    return *(cur_++) == ch;
  }
  inline bool Eat(uint8_t ch0, uint8_t ch1) {
    if (Remaining() < 2)
      return false;
    if (cur_[0] != ch0 || cur_[1] != ch1)
      return false;
    cur_ += 2;
    return true;
  }
  inline bool Eat(uint8_t ch0, uint8_t ch1, uint8_t ch2, uint8_t ch3) {
    if (Remaining() < 4)
      return false;
    if (cur_[0] != ch0 || cur_[1] != ch1 || cur_[2] != ch2 || cur_[3] != ch3)
      return false;
    cur_ += 4;
    return true;
  }

 private:
  Region region_;
  Region::iterator cur_;
};

}  // namespace zucchini

#endif  // ZUCCHINI_REGION_H_
