// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef TraceWrapperPersistent_h
#define TraceWrapperPersistent_h

#include "platform/bindings/ScriptWrappableVisitor.h"
#include "platform/heap/HeapAllocator.h"

namespace blink {

template <typename T>
class Persistent;

// TraceWrapperPersistent is used for |Persistent|s that should participate in
// wrapper tracing, i.e., have the same semantics as |Persistent|s but are
// pointing to ScriptWrappable and thus require write barrier support.
//
// TraceWraperPersistent still need to be traced from some object in
// |TraceWrappers|!
//
// Note that TraceWrapperPersisntent is the counter part to regular Persistent,
// which means it has to be used on the same thread.
template <typename T>
class TraceWrapperPersistent
    : public PersistentBase<T,
                            kNonWeakPersistentConfiguration,
                            kSingleThreadPersistentConfiguration> {
  typedef PersistentBase<T,
                         kNonWeakPersistentConfiguration,
                         kSingleThreadPersistentConfiguration>
      Parent;

 public:
  TraceWrapperPersistent() : Parent() {}
  TraceWrapperPersistent(std::nullptr_t) : Parent(nullptr) {}

  TraceWrapperPersistent(T* raw) : Parent(raw) {
    ScriptWrappableVisitor::WriteBarrier(this->Get());
  }

  TraceWrapperPersistent(WTF::HashTableDeletedValueType x) : Parent(x) {}

  TraceWrapperPersistent& operator=(std::nullptr_t) {
    Parent::operator=(nullptr);
    return *this;
  }

  TraceWrapperPersistent& operator=(const TraceWrapperPersistent& other) {
    Parent::operator=(other);
    ScriptWrappableVisitor::WriteBarrier(this->Get());
    return *this;
  }
};

}  // namespace blink

#endif  // TraceWrapperPersistent_h
