// Copyright (c) 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef WTF_ConstructTraits_h
#define WTF_ConstructTraits_h

#include "platform/wtf/Allocator.h"
#include "platform/wtf/TypeTraits.h"
#include "platform/wtf/VectorTraits.h"

namespace WTF {

template <typename T>
class ConstructTraitsBase {
  STATIC_ONLY(ConstructTraitsBase);

 protected:
  template <typename... Args>
  static T* ConstructElementInternal(void* location, Args&&... args) {
    return new (NotNull, location) T(std::forward<Args>(args)...);
  }
};

// ConstructTraits is used to construct elements in WTF collections. Instead of
// constructing elements using placement new, they should be constructed using
// ConstructTraits.
template <typename T,
          typename Allocator,
          bool = VectorTraits<T>::template IsTraceableInCollection<>::value>
class ConstructTraits;

template <typename T, typename Allocator>
class ConstructTraits<T, Allocator, false /*IsTraceableInCollection*/>
    : public ConstructTraitsBase<T> {
  STATIC_ONLY(ConstructTraits);

 public:
  template <typename... Args>
  static T* ConstructElement(void* location, Args&&... args) {
    return ConstructTraitsBase<T>::ConstructElementInternal(
        location, std::forward<Args>(args)...);
  }

  static void ConstructElements(T* array, size_t len) {}
};

template <typename T, typename Allocator>
class ConstructTraits<T, Allocator, true /*IsTraceableInCollection*/>
    : public ConstructTraitsBase<T> {
  STATIC_ONLY(ConstructTraits);

 public:
  template <typename... Args>
  static T* ConstructElement(void* location, Args&&... args) {
    T* object = ConstructTraitsBase<T>::ConstructElementInternal(
        location, std::forward<Args>(args)...);
    Allocator::template NotifyNewObject<T, VectorTraits<T>>(object);
    return object;
  }

  static void ConstructElements(T* array, size_t len) {
    Allocator::template NotifyNewObjects<T, VectorTraits<T>>(array, len);
  }
};

}  // namespace WTF

#endif  // WTF_ConstructTraits_h
