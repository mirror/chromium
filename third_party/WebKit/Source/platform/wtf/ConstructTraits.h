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

template <typename T,
          typename Allocator,
          bool = Allocator::kIsGarbageCollected,
          bool = VectorTraits<T>::template IsTraceableInCollection<>::value>
class ConstructTraits;

template <typename T, typename Allocator>
class ConstructTraits<T,
                      Allocator,
                      false /*kIsGarbageCollected*/,
                      false /*IsTraceableInCollection*/>
    : public ConstructTraitsBase<T> {
  STATIC_ONLY(ConstructTraits);

 public:
  template <typename... Args>
  static T* ConstructElement(void* location, Args&&... args) {
    return ConstructTraitsBase<T>::ConstructElementInternal(
        location, std::forward<Args>(args)...);
  }

  static void NewInitializedElements(T* begin, T* end) {}
};

template <typename T, typename Allocator>
class ConstructTraits<T,
                      Allocator,
                      true /*kIsGarbageCollected*/,
                      false /*IsTraceableInCollection*/>
    : public ConstructTraitsBase<T> {
  STATIC_ONLY(ConstructTraits);

 public:
  template <typename... Args>
  static T* ConstructElement(void* location, Args&&... args) {
    return ConstructTraitsBase<T>::ConstructElementInternal(
        location, std::forward<Args>(args)...);
  }

  static void NewInitializedElements(T* begin, T* end) {}
};

// Special case for WTF::Vector<WTF::Vector<T>> where the inner vector is
// actually tracable but the whole data structure is allocated on an
// unmanaged heap.
template <typename T, typename Allocator>
class ConstructTraits<T,
                      Allocator,
                      false /*kIsGarbageCollected*/,
                      true /*IsTraceableInCollection*/>
    : public ConstructTraitsBase<T> {
  STATIC_ONLY(ConstructTraits);

 public:
  template <typename... Args>
  static T* ConstructElement(void* location, Args&&... args) {
    return ConstructTraitsBase<T>::ConstructElementInternal(
        location, std::forward<Args>(args)...);
  }

  static void NewInitializedElements(T* begin, T* end) {}
};

template <typename T, typename Allocator>
class ConstructTraits<T,
                      Allocator,
                      true /*kIsGarbageCollected*/,
                      true /*IsTraceableInCollection*/>
    : public ConstructTraitsBase<T> {
  STATIC_ONLY(ConstructTraits);

 public:
  template <typename... Args>
  static T* ConstructElement(void* location, Args&&... args) {
    T* tmp = ConstructTraitsBase<T>::ConstructElementInternal(
        location, std::forward<Args>(args)...);
    Allocator::template NotifyNewObject<T, VectorTraits<T>>(tmp);
    return tmp;
  }

  static void NewInitializedElements(T* begin, T* end) {
    Allocator::template NotifyNewObjects<T, VectorTraits<T>>(begin, end);
  }
};

}  // namespace WTF

#endif  // WTF_ConstructTraits_h
