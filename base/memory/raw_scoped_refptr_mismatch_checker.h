// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef BASE_MEMORY_RAW_SCOPED_REFPTR_MISMATCH_CHECKER_H_
#define BASE_MEMORY_RAW_SCOPED_REFPTR_MISMATCH_CHECKER_H_

#include <tuple>
#include <type_traits>

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

template <typename T, typename SFINAE = void>
struct IsRefCountedType : std::false_type {};

template <typename T>
struct IsRefCountedType<T, void_t<decltype(&T::AddRef), decltype(&T::Release)>>
    : std::true_type {};

template <typename T>
struct NeedsScopedRefptrButGetsRawPtr {
  using U = typename std::decay<T>::type;

  static constexpr bool value =
      std::is_pointer<U>::value &&
      IsRefCountedType<typename std::remove_pointer<U>::type>::value;
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
