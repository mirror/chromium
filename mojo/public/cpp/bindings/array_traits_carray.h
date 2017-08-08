// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MOJO_PUBLIC_CPP_BINDINGS_ARRAY_TRAITS_CARRAY_H_
#define MOJO_PUBLIC_CPP_BINDINGS_ARRAY_TRAITS_CARRAY_H_

#include "mojo/public/cpp/bindings/array_traits.h"

namespace mojo {

template <typename T>
struct CArray {
  CArray() : size(0), data(nullptr) {}
  CArray(size_t size, T* data) : size(size), data(data) {}
  const size_t size;
  T* const data;
};

template <typename T>
struct ConstCArray {
  ConstCArray() : size(0), data(nullptr) {}
  ConstCArray(size_t size, const T* data) : size(size), data(data) {}
  const size_t size;
  const T* const data;
};

template <typename T>
struct ArrayTraits<CArray<T>> {
  using Element = T;

  static size_t GetSize(const CArray<T>& input) { return input.size; }

  static T* GetData(CArray<T>& input) { return input.data; }

  static const T* GetData(const CArray<T>& input) { return input.data; }

  static T& GetAt(CArray<T>& input, size_t index) { return input.data[index]; }

  static const T& GetAt(const CArray<T>& input, size_t index) {
    return input.data[index];
  }

  static bool Resize(CArray<T>& input, size_t size) {
    return size <= input.size;
  }
};

template <typename T>
struct ArrayTraits<ConstCArray<T>> {
  using Element = T;

  static bool IsNull(const ConstCArray<T>& input) { return !input.data; }

  static size_t GetSize(const ConstCArray<T>& input) { return input.size; }

  static const T* GetData(const ConstCArray<T>& input) { return input.data; }

  static const T& GetAt(const ConstCArray<T>& input, size_t index) {
    return input.data[index];
  }
};

}  // namespace mojo

#endif  // MOJO_PUBLIC_CPP_BINDINGS_ARRAY_TRAITS_CARRAY_H_
