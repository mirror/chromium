// Copyright (c) 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IDLTypeTraits_h
#define IDLTypeTraits_h

#include <type_traits>
#include "bindings/core/v8/NativeValueTraits.h"
#include "platform/bindings/V8Binding.h"
#include "platform/heap/Handle.h"
#include "platform/wtf/TypeTraits.h"

namespace blink {

// Given an IDL type IDLType, sets |type| to either IDLType's corresponding C++
// type or to Member<cpp type>, depending on whether IDLType is a
// garbage-collected type.
template <typename IDLType>
struct AddMember final {
 private:
  using CppType = typename NativeValueTraits<IDLType>::ImplType;

 public:
  using type =
      typename std::conditional<WTF::IsGarbageCollectedType<CppType>::value,
                                Member<CppType>,
                                CppType>::type;
};

// Given an IDL type IDLType, sets |value| to true if IDLType's C++ type needs
// to be used with a HeapVector rather than a regular Vector.
//
// Two kinds of types need HeapVector:
// 1. Oilpan types, which are wrapped by Member<>.
// 2. IDL unions and dictionaries, which are not Oilpan types themselves (and
//    are thus not wrapped by Member<>) but have a trace() method and can
//    contain Oilpan members. They both also happen to specialize V8TypeOf,
//    which we use to recognize them.
template <typename IDLType>
struct NeedsHeapVector final {
 private:
  using CppType = typename NativeValueTraits<IDLType>::ImplType;

 public:
  static constexpr bool value =
      WTF::IsGarbageCollectedType<CppType>::value ||
      std::is_class<typename V8TypeOf<CppType>::Type>::value;
};

// Given an IDL type IDLType, sets |type| to a HeapVector<cpp type>, a
// HeapVector<Member<cpp type>> or a Vector<cpp type> depending on IDLType's
// corresponding C++ type.
template <typename IDLType>
struct WrapInVectorType final {
 private:
  using CppType = typename NativeValueTraits<IDLType>::ImplType;

 public:
  using type =
      typename std::conditional<NeedsHeapVector<IDLType>::value,
                                HeapVector<typename AddMember<IDLType>::type>,
                                Vector<CppType>>::type;
};

}  // namespace blink

#endif  // IDLTypeTraits_h
