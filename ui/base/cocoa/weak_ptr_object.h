// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_BASE_COCOA_WEAK_PTR_OBJECT_H_
#define UI_BASE_COCOA_WEAK_PTR_OBJECT_H_

#include "base/macros.h"
#include "ui/base/ui_base_export.h"

#if defined(__OBJC__)
@class WeakPtrObject;
#else
class WeakPtrObject;
#endif

namespace ui {
namespace internal {

class UI_BASE_EXPORT WeakPtrObjectFactoryBase {
 protected:
  static WeakPtrObject* Create(void* owner);
  static void* UnWrap(WeakPtrObject* handle);
  static void InvalidateAndRelease(WeakPtrObject* handle);
};

}  // namespace internal

// Class that wraps a single WeakPtr in an NSObject, which can be ref counted.
// Use this like WeakPtrFactory.
template <class T>
class WeakPtrObjectFactory : public internal::WeakPtrObjectFactoryBase {
 public:
  explicit WeakPtrObjectFactory(T* owner) : handle_(Create(owner)) {}
  ~WeakPtrObjectFactory() { InvalidateAndRelease(handle_); }

  // Gets the original owner, if it hasn't been destroyed.
  static T* Get(WeakPtrObject* p) { return static_cast<T*>(UnWrap(p)); }

  // Gets the NSObject, which can then be retained.
  WeakPtrObject* handle() { return handle_; }

 private:
  WeakPtrObject* handle_;

  DISALLOW_COPY_AND_ASSIGN(WeakPtrObjectFactory);
};

}  // namespace ui

#endif  // UI_BASE_COCOA_WEAK_PTR_OBJECT_H_
