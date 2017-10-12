// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef BASE_MEMORY_RAW_SCOPED_REFPTR_MISMATCH_CHECKER_H_
#define BASE_MEMORY_RAW_SCOPED_REFPTR_MISMATCH_CHECKER_H_

#include <tuple>
#include <type_traits>

#include "base/memory/ref_counted.h"

// It is dangerous to post a task with a T* argument where T is a subtype of
// RefCounted(Base|ThreadSafeBase), since by the time the parameter is used, the
// object may already have been deleted since it was not held with a
// scoped_refptr. Example: http://crbug.com/27191
// The following set of traits are designed to generate a compile error
// whenever this antipattern is attempted.

namespace base {

// This is a base internal implementation file used by task.h and callback.h.
// Not for public consumption, so we wrap it in namespace internal.
namespace internal {

// TODO: this is generic.
// Catch any other type of pointer.
struct TypeIsBindable {};
template <typename v, typename T>
TypeIsBindable IsTypeAsPtrUnbindable(const void*) {
  return TypeIsBindable();
}
template <typename v, typename T, typename ReturnType, typename... Args>
TypeIsBindable IsTypeAsPtrUnbindable(ReturnType (*)(Args...)) {
  // just to make sure that this isn't matching too much
  return TypeIsBindable();
}

// We do this since template specializations don't match subclasses.  Plus,
// static_assert requires a string constant, so it pretty much has to be a
// macro if we want to provide custom messages.
#define UNBINDABLE_AS_PTR_REASON(RuleName, TypeName, Reason)          \
  template <typename v>                                               \
  struct RuleName##_IsUnbindable {                                    \
    static_assert(v::value, Reason);                                  \
  };                                                                  \
  template <typename v, typename T>                                   \
  RuleName##_IsUnbindable<v> IsTypeAsPtrUnbindable(const TypeName*) { \
    return RuleName##_IsUnbindable<v>();                              \
  }

UNBINDABLE_AS_PTR_REASON(
    RefCountedBase,
    subtle::RefCountedBase,
    "A parameter is a refcounted type and needs scoped_refptr.");
UNBINDABLE_AS_PTR_REASON(
    RefCountedThreadSafeBase,
    subtle::RefCountedThreadSafeBase,
    "A parameter is a refcounted type and needs scoped_refptr.");

template <typename T>
struct NeedsScopedRefptrButGetsRawPtr {
  static_assert(!std::is_reference<T>::value,
                "NeedsScopedRefptrButGetsRawPtr requires non-reference type.");

  enum {
    // Human readable translation: you needed to be a scoped_refptr if you are a
    // raw pointer type and are convertible to a RefCounted(Base|ThreadSafeBase)
    // type.
    value =
        (std::is_pointer<T>::value &&
         (std::is_convertible<T, const subtle::RefCountedBase*>::value ||
          std::is_convertible<T,
                              const subtle::RefCountedThreadSafeBase*>::value))
  };
};

template <typename Params>
struct ParamsUseScopedRefptrCorrectly {
  enum { value = 0 };
};

template <>
struct ParamsUseScopedRefptrCorrectly<std::tuple<>> {
  enum { value = 1 };
};

template <typename Head, typename... Tail>
struct ParamsUseScopedRefptrCorrectly<std::tuple<Head, Tail...>> {
  enum { value = !NeedsScopedRefptrButGetsRawPtr<Head>::value &&
                  ParamsUseScopedRefptrCorrectly<std::tuple<Tail...>>::value };
};

}  // namespace internal

}  // namespace base

#endif  // BASE_MEMORY_RAW_SCOPED_REFPTR_MISMATCH_CHECKER_H_
