// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef BASE_NO_DESTRUCTOR_H_
#define BASE_NO_DESTRUCTOR_H_

#include <utility>

namespace base {

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
