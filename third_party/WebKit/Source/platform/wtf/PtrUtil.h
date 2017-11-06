// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_WEBKIT_SOURCE_PLATFORM_WTF_PTRUTIL_H_
#define THIRD_PARTY_WEBKIT_SOURCE_PLATFORM_WTF_PTRUTIL_H_

#include <memory>
#include <type_traits>

#include "base/memory/ptr_util.h"
#include "platform/wtf/TypeTraits.h"

namespace WTF {

template <typename T>
std::unique_ptr<T> WrapUnique(T* ptr) {
  static_assert(
      !WTF::IsGarbageCollectedType<T>::value,
      "Garbage collected types should not be stored in std::unique_ptr!");
  return std::unique_ptr<T>(ptr);
}

template <typename T>
std::unique_ptr<T[]> WrapArrayUnique(T* ptr) {
  static_assert(
      !WTF::IsGarbageCollectedType<T>::value,
      "Garbage collected types should not be stored in std::unique_ptr!");
  return std::unique_ptr<T[]>(ptr);
}

}  // namespace WTF

using WTF::WrapArrayUnique;

#endif  // THIRD_PARTY_WEBKIT_SOURCE_PLATFORM_WTF_PTRUTIL_H_
