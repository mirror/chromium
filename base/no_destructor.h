// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef BASE_NO_DESTRUCTOR_H_
#define BASE_NO_DESTRUCTOR_H_

#include <utility>

namespace base {

// A wrapper that makes it easy to create an object of type T with static
// storage duration that:
// - is only constructed on first access
// - never invokes the destructor
// in order to satisfy the styleguide ban on global constructors and
// destructors. Example usage:
//
// T& GetT() {
//   static NoDestructor<T> value(arg1, arg2, arg3...);
//   return *value;
// }
//
// NoDestructor<T> stores the object inline, so it also avoids a pointer
// indirection and a malloc. It should be preferred over both:
// - CR_DEFINE_STATIC_LOCAL();
// - static auto* value = new X(arg1, arg2, arg3, ...);
//
// Note that since the destructor is never run, this *will* leak memory if used
// as a stack or member variable. Furthermore, a NoDestructor<T> should never
// have global scope, as requires a static initializer.
template <typename T>
class NoDestructor {
 public:
  template <typename... Args>
  explicit NoDestructor(Args&&... args)
      : storage_(std::forward<Args>(args)...) {}

  ~NoDestructor() {
    // |storage_| is part of a union, so its destructor is skipped unless
    // manually invoked.
  }

  const T& operator*() const { return storage_; }
  T& operator*() { return storage_; }

  const T* operator->() const { return &storage_; }
  T* operator->() { return &storage_; }

  const T* get() const { return &storage_; }
  T* get() { return &storage_; }

 private:
  union {
    T storage_;
  };
};

}  // namespace base

#endif  // BASE_MEMORY_PTR_UTIL_H_
