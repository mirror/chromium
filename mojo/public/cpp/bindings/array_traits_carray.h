// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MOJO_PUBLIC_CPP_BINDINGS_ARRAY_TRAITS_CARRAY_H_
#define MOJO_PUBLIC_CPP_BINDINGS_ARRAY_TRAITS_CARRAY_H_

#include "mojo/public/cpp/bindings/array_traits.h"

namespace mojo {

template <typename T>
class CArray {
 public:
  constexpr CArray() : size_(0), data_(nullptr) {}
  constexpr CArray(size_t size, T* data) : size_(size), data_(data) {}

  constexpr size_t size() const noexcept { return size_; }
  constexpr T* data() const noexcept { return data_; }

 private:
  size_t size_;
  T* data_;
};

template <typename T>
class ConstCArray {
 public:
  ConstCArray() : size_(0), data_(nullptr) {}
  ConstCArray(size_t size, const T* data) : size_(size), data_(data) {}

  constexpr size_t size() const noexcept { return size_; }
  constexpr const T* data() const noexcept { return data_; }

 private:
  size_t size_;
  const T* data_;
};

template <typename T>
struct ArrayTraits<CArray<T>> {
  using Element = T;

  static size_t GetSize(const CArray<T>& input) { return input.size(); }

  static T* GetData(CArray<T>& input) { return input.data(); }

  static const T* GetData(const CArray<T>& input) { return input.data(); }

  static T& GetAt(CArray<T>& input, size_t index) {
    return input.data()[index];
  }

  static const T& GetAt(const CArray<T>& input, size_t index) {
    return input.data()[index];
  }

  static bool Resize(CArray<T>& input, size_t size) {
    return size <= input.size();
  }
};

template <typename T>
struct ArrayTraits<ConstCArray<T>> {
  using Element = T;

  static size_t GetSize(const ConstCArray<T>& input) { return input.size(); }

  static const T* GetData(const ConstCArray<T>& input) { return input.data(); }

  static const T& GetAt(const ConstCArray<T>& input, size_t index) {
    return input.data()[index];
  }
};

}  // namespace mojo

#endif  // MOJO_PUBLIC_CPP_BINDINGS_ARRAY_TRAITS_CARRAY_H_
