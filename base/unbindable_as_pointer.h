// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef BASE_UNBINDABLE_AS_POINTER_H_
#define BASE_UNBINDABLE_AS_POINTER_H_

#include <type_traits>

#include "base/memory/weak_ptr.h"

namespace base {

// Anything that is an UnbindableAsPointer may not be provided as a pointer of
// any kind to a bound functor.  For example, neither of the Bind()s will work:
// void MyFunction(UnbindableAsPointer* arg) { ... }
// base::Bind(&MyFunction)
// base::Bind(&MyFunction, nullptr)
//
// The same holds for WeakPtr<UnbindableAsPointer> and unique_ptr<>.
struct UnbindableAsPointer {};

namespace internal {

// Helpers for base::Bind.

// Set |value| to |true_type| if |T| is used incorrectly.  We want to prevent
// binding any Functor that takes a pointer to an UnbindableAsPointer.
template <typename T>
struct DoesIncorrectlyUseUnbindableAsPointerType {
  template <typename Type>
  struct ExtractBaseType {
    using type = Type;
  };
  template <typename Type>
  struct ExtractBaseType<Type*> : ExtractBaseType<Type> {};

  template <typename Type>
  struct IsWeakPtr : std::false_type {};
  template <typename Type>
  struct IsWeakPtr<WeakPtr<Type>>
      : std::is_convertible<Type, UnbindableAsPointer> {};

  enum {
    // If |T| is a pointer, with any level of indirection, to an
    // UnbindableAsPointer, then return true.  Also disallow WeakPtr<> and
    // unique_ptr to them.
    value = (std::is_pointer<T>::value &&
             std::is_convertible<typename ExtractBaseType<T>::type,
                                 UnbindableAsPointer>::value) ||
            std::is_convertible<std::decay_t<T>,
                                std::unique_ptr<UnbindableAsPointer>>::value ||
            IsWeakPtr<std::decay_t<T>>::value
  };
};

}  // namespace internal

}  // namespace base

#endif  // BASE_UNBINDABLE_AS_POINTER_H_
