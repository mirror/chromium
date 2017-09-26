// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MOJO_PUBLIC_CPP_BINDINGS_TRACKED_SCOPED_REFPTR_H_
#define MOJO_PUBLIC_CPP_BINDINGS_TRACKED_SCOPED_REFPTR_H_

#include <cinttypes>

#include "base/debug/alias.h"
#include "base/debug/crash_logging.h"
#include "base/strings/stringprintf.h"

namespace mojo {

// TODO(crbug.com/750267, crbug.com/754946): Remove this after bug
// investigation.
template <typename T>
class TrackedScopedRefPtr {
 public:
  TrackedScopedRefPtr() {}

  TrackedScopedRefPtr(T* ptr) : ptr_(ptr), ptr_copy_(ptr) {}

  TrackedScopedRefPtr(const TrackedScopedRefPtr<T>& other)
      : ptr_(other.ptr_), ptr_copy_(other.ptr_copy_) {}

  TrackedScopedRefPtr(TrackedScopedRefPtr<T>&& other)
      : ptr_(std::move(other.ptr_)), ptr_copy_(other.ptr_copy_) {
    other.ptr_copy_ = nullptr;
  }

  ~TrackedScopedRefPtr() { state_ = DESTROYED; }

  T* get() const { return ptr_.get(); }

  T& operator*() const { return *ptr_; }

  T* operator->() const { return ptr_.get(); }

  TrackedScopedRefPtr<T>& operator=(T* ptr) {
    ptr_ = ptr;
    ptr_copy_ = ptr;
    return *this;
  }

  TrackedScopedRefPtr<T>& operator=(const TrackedScopedRefPtr<T>& other) {
    ptr_ = other.ptr_;
    ptr_copy_ = other.ptr_copy_;
    return *this;
  }

  TrackedScopedRefPtr<T>& operator=(TrackedScopedRefPtr<T>&& other) {
    ptr_ = std::move(other.ptr_);
    ptr_copy_ = other.ptr_copy_;
    other.ptr_copy_ = nullptr;
    return *this;
  }

  void CheckObjectIsValid() {
    CHECK(this);

    LifeState state = state_;
    base::debug::Alias(&state);

    T* ptr = ptr_.get();
    base::debug::Alias(&ptr);

    T* ptr_copy = ptr_copy_;
    base::debug::Alias(&ptr_copy);

    if (state_ != ALIVE || !ptr_ || ptr_.get() != ptr_copy_) {
      std::string value = base::StringPrintf(
          "%" PRIx64 " %" PRIxPTR " %" PRIxPTR, static_cast<uint64_t>(state_),
          reinterpret_cast<uintptr_t>(ptr_.get()),
          reinterpret_cast<uintptr_t>(ptr_copy_));
      base::debug::SetCrashKeyValue("tracked_scoped_refptr_state", value);
      CHECK(false);
    }
  }

 private:
  enum LifeState : uint64_t {
    ALIVE = 0x1029384756afbecd,
    DESTROYED = 0xdcebfa6574839201
  };

  LifeState state_ = ALIVE;
  scoped_refptr<T> ptr_;
  T* ptr_copy_ = nullptr;
};

}  // namespace mojo

#endif  // MOJO_PUBLIC_CPP_BINDINGS_TRACKED_SCOPED_REFPTR_H_
