// Copyright (c) 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef HeapTraits_h
#define HeapTraits_h

#include <type_traits>
#include "platform/heap/HeapAllocator.h"
#include "platform/heap/Member.h"
#include "platform/wtf/TypeTraits.h"

namespace blink {

// Given a type T, sets |type| to either Member<T> or just T depending on
// whether T is a garbage-collected type.
template <typename T>
struct MaybeMember final {
  using type = typename std::
      conditional<WTF::IsGarbageCollectedType<T>::value, Member<T>, T>::type;
};

// Given a type T, sets |value| to true if T needs to be used with a HeapVector
// rather than a regular Vector.
//
// Two kinds of types need HeapVector:
// 1. Oilpan types, which are wrapped by Member<>.
// 2. IDL unions and dictionaries, which are not Oilpan types themselves (and
//    are thus not wrapped by Member<>) but have a Trace() method and can
//    contain Oilpan members.
template <typename T>
struct NeedsHeapVector final {
  static constexpr bool value = WTF::IsTraceable<T>::value;
};

// Given a type T, sets |type| to a HeapVector<T>, HeapVector<Member<T>> or
// Vector<T> depending on T.
template <typename T>
struct VectorOf final {
  using type =
      typename std::conditional<NeedsHeapVector<T>::value,
                                HeapVector<typename MaybeMember<T>::type>,
                                Vector<T>>::type;
};

// Given a type T, sets |type| to a HeapVector<T>, HeapVector<Member<T>> or
// Vector<T> depending on T.
template <typename T, typename U>
struct VectorOfPairs final {
  using type = typename std::conditional<
      NeedsHeapVector<T>::value || NeedsHeapVector<U>::value,
      HeapVector<std::pair<typename MaybeMember<T>::type,
                           typename MaybeMember<U>::type>>,
      Vector<std::pair<T, U>>>::type;
};

}  // namespace blink

#endif  // HeapTraits_h
