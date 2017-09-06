// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_COMMON_SHARED_INTERFACE_PTR_H_
#define CONTENT_COMMON_SHARED_INTERFACE_PTR_H_

namespace content {

#include "base/memory/ref_counted.h"
#include "mojo/public/cpp/bindings/interface_ptr.h"

// A helper class that allows multiple owners to share an interface pointer.
template <typename T>
class SharedInterfacePtr : public base::RefCounted<SharedInterfacePtr<T>> {
 public:
  SharedInterfacePtr() {}
  explicit SharedInterfacePtr(mojo::InterfacePtr<T> ptr)
      : ptr_(std::move(ptr)) {}

  static scoped_refptr<SharedInterfacePtr> CreateForTesting(T* raw_ptr) {
    return make_scoped_refptr(new SharedInterfacePtr(raw_ptr));
  }

  void Bind(mojo::InterfacePtrInfo<T> ptr_info,
            scoped_refptr<base::SingleThreadTaskRunner> runner = nullptr) {
    ptr_.Bind(std::move(ptr_info), std::move(runner));
  }

  T* get() const {
    if (raw_ptr_for_testing_)
      return raw_ptr_for_testing_;
    return ptr_.get();
  }
  T* operator->() const { return get(); }
  T& operator*() const { return *get(); }
  explicit operator bool() const { return get(); }

 private:
  friend class base::RefCounted<SharedInterfacePtr>;
  ~SharedInterfacePtr() {}

  explicit SharedInterfacePtr(T* raw_ptr) : raw_ptr_for_testing_(raw_ptr) {}

  T* raw_ptr_for_testing_ = nullptr;
  mojo::InterfacePtr<T> ptr_;
};

}  // namespace content

#endif  // CONTENT_COMMON_SHARED_INTERFACE_PTR_H_
