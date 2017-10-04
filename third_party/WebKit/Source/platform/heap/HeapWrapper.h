// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef HeapWrapper_h
#define HeapWrapper_h

#include "platform/heap/GarbageCollected.h"

namespace blink {

// The HeapWrapper class can wrap an object that is marked with STACK_ALLOCATED,
// DISALLOW_NEW or DISALLOW_NEW_EXCEPT_PLACEMENT_NEW so that it can be stored on
// the heap.
//
// This is useful when such an object needs to be bound into a WTF::Function.
template <typename T>
class HeapWrapper : public GarbageCollectedFinalized<HeapWrapper<T>> {
 public:
  HeapWrapper(T value) : value_(value) {}

  const T& value() const { return value_; }
  T& value() { return value_; }

  DEFINE_INLINE_TRACE() { visitor->Trace(value_); }

 private:
  T value_;
};

template <typename T>
static HeapWrapper<T>* MakeHeapWrapper(T value) {
  return new HeapWrapper<T>(value);
}

}  // namespace blink

#endif  // HeapWrapper_h
