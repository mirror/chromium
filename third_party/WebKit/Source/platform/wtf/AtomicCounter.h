// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef AtomicCounter_h
#define AtomicCounter_h

#include <atomic>

#include "platform/wtf/WTFExport.h"

namespace WTF {

// Maintains an atomic count which can be incremented, decremented, and read.
//
// No ordering of this variable relative to other counters or other memory is
// guaranteed; external synchronization is required if you require a
// "consistent" view of counts from other threads.
class AtomicCounter {
 public:
  constexpr AtomicCounter() : count_(0) {}

  int GetCount() const { return count_.load(std::memory_order_relaxed); }
  void Increment() { count_.fetch_add(1, std::memory_order_relaxed); }
  void Decrement() { count_.fetch_sub(1, std::memory_order_relaxed); }

 private:
  std::atomic_int count_;
};

}  // namespace WTF

#endif  // AtomicCounter_h
