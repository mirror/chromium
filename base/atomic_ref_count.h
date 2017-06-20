// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// This is a low level implementation of atomic semantics for reference
// counting.  Please use base/memory/ref_counted.h directly instead.

#ifndef BASE_ATOMIC_REF_COUNT_H_
#define BASE_ATOMIC_REF_COUNT_H_

#include <atomic>

namespace base {

class AtomicRefCount {
 public:
  constexpr AtomicRefCount() : ref_count_(0) {}
  constexpr AtomicRefCount(int initial_value) : ref_count_(initial_value) {}

  // Increment a reference count.
  void Increment() { Increment(1); }

  // Increment a reference count by "increment", which must exceed 0.
  void Increment(int increment) {
    ref_count_.fetch_add(increment, std::memory_order_relaxed);
  }

  // Decrement a reference count, and return whether the result is non-zero.
  // Insert barriers to ensure that state written before the reference count
  // became zero will be visible to a thread that has just made the count zero.
  bool Decrement() {
    if (ref_count_.fetch_sub(1, std::memory_order_release) == 1) {
      AcquireFence();
      return false;
    }
    return true;
  }

  // Return whether the reference count is one.  If the reference count is used
  // in the conventional way, a refrerence count of 1 implies that the current
  // thread owns the reference and no other thread shares it.  This call
  // performs the test for a reference count of one, and performs the memory
  // barrier needed for the owning thread to act on the object, knowing that it
  // has exclusive access to the object.
  bool IsOne() const {
    if (ref_count_.load(std::memory_order_relaxed) == 1) {
      AcquireFence();
      return true;
    }
    return false;
  }

  // Return whether the reference count is zero.  With conventional object
  // referencing counting, the object will be destroyed, so the reference count
  // should never be zero.  Hence this is generally used for a debug check.
  bool IsZero() const {
    if (ref_count_.load(std::memory_order_relaxed) == 0) {
      AcquireFence();
      return true;
    }
    return false;
  }

  // Returns the current reference count (with no barriers). This is subtle, and
  // should be used only for debugging.
  int RefCount() const { return ref_count_.load(std::memory_order_relaxed); }

 private:
  inline void AcquireFence() const {
    // TODO(jbroman): This should really be
    //   std::atomic_thread_fence(std::memory_order_acquire)
    // but TSAN doesn't recognize that as synchronizing with a release store to
    // |ref_count_|.
    ref_count_.load(std::memory_order_acquire);
  }

  std::atomic_int ref_count_;
};

// TODO(jbroman): Inline these functions once the aboe changes stick.

// Increment a reference count by "increment", which must exceed 0.
inline void AtomicRefCountIncN(volatile AtomicRefCount* ptr, int increment) {
  const_cast<AtomicRefCount*>(ptr)->Increment(increment);
}

// Increment a reference count by 1.
inline void AtomicRefCountInc(volatile AtomicRefCount *ptr) {
  const_cast<AtomicRefCount*>(ptr)->Increment();
}

// Decrement a reference count by 1 and return whether the result is non-zero.
// Insert barriers to ensure that state written before the reference count
// became zero will be visible to a thread that has just made the count zero.
inline bool AtomicRefCountDec(volatile AtomicRefCount *ptr) {
  return const_cast<AtomicRefCount*>(ptr)->Decrement();
}

// Return whether the reference count is one.  If the reference count is used
// in the conventional way, a refrerence count of 1 implies that the current
// thread owns the reference and no other thread shares it.  This call performs
// the test for a reference count of one, and performs the memory barrier
// needed for the owning thread to act on the object, knowing that it has
// exclusive access to the object.
inline bool AtomicRefCountIsOne(volatile AtomicRefCount *ptr) {
  return const_cast<AtomicRefCount*>(ptr)->IsOne();
}

// Return whether the reference count is zero.  With conventional object
// referencing counting, the object will be destroyed, so the reference count
// should never be zero.  Hence this is generally used for a debug check.
inline bool AtomicRefCountIsZero(volatile AtomicRefCount *ptr) {
  return const_cast<AtomicRefCount*>(ptr)->IsZero();
}

}  // namespace base

#endif  // BASE_ATOMIC_REF_COUNT_H_
