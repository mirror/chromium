// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef LockFreeMap_h
#define LockFreeMap_h

#include <unordered_map>

#include "base/synchronization/lock.h"
#include "platform/PlatformExport.h"

namespace blink {

using base::subtle::Atomic32;

// The thread-safe wrapper over std::unordered_map.
template<typename T>
class PLATFORM_EXPORT LockFreeMap {
 public:
  LockFreeMap();

  void Insert(void* address, T&& value) {
    // unordered_map::insert does not invalidate iterators unless it performs
    // rehashing. The lock-free logic below makes sure the fast-path insertion
    // does not trigger rehashing.

    {
      AtomicEntryCounter entry_counter(&operations_in_flight);
      Atomic32 insertions_left = base::subtle::Barrier_AtomicIncrement(
          &insertions_left_till_rehashing_, -1);
      if (LIKELY(insertions_left > 0)) {
        map_.insert(std::make_pair<void*, T>(std::move(value)));
        return;
      }
    }

    InsertSlow(address, value);
  }

  bool Has(void* address) {
    {
      AtomicEntryCounter entry_counter(&operations_in_flight);
      if (LIKELY(base::subtle::Acquire_Load(&insertions_left_till_rehashing_) > 0))
        return map_.find(address) != map_.end();
    }
    base::AutoLock lock(mutex_);
    return map_.find(address) != map_.end();
  }

  void Remove(void* address) {
    {
      AtomicEntryCounter entry_counter(&operations_in_flight);
      if (LIKELY(base::subtle::Acquire_Load(&insertions_left_till_rehashing_) > 0)) {
        map_.erase(address);
        // !!! what if it goes 0 => 1 ???
        base::subtle::Barrier_AtomicIncrement(&insertions_left_till_rehashing_, 1);
        return;
      }
    }
    base::AutoLock lock(mutex_);
    map_.erase(address);
  }

  base::Lock& lock() { return mutex_; }
  std::unordered_map<void*, T> map() { return map_; }

 private:
  class AtomicEntryCounter {
   public:
    AtomicEntryCounter(Atomic32* counter) : counter_(counter) {
      base::subtle::NoBarrier_AtomicIncrement(counter_, 1);
    }

    ~AtomicEntryCounter() {
      base::subtle::NoBarrier_AtomicIncrement(counter_, -1);
    }

   private:
    Atomic32* counter_;
  };

  void InsertSlow(void* address, T&& value);

  std::unordered_map<void*, T> map_;

  base::Lock mutex_;
  Atomic32 insertions_left_till_rehashing_;
  Atomic32 operations_in_flight;

  static const size_t kInitialBucketCount = 1000;

  DISALLOW_COPY_AND_ASSIGN(LockFreeMap);
};

template<typename T>
LockFreeMap<T>::LockFreeMap()
    : map_(kInitialBucketCount) {
  int insertions_till_rehashing = static_cast<int>(
      map_.max_load_factor() * map_.bucket_count() - map_.size());
  base::subtle::NoBarrier_Store(&insertions_left_till_rehashing_,
                                insertions_till_rehashing);
  base::subtle::NoBarrier_Store(&operations_in_flight, 0);
}

template<typename T>
void LockFreeMap<T>::InsertSlow(void* address, T&& value) {
  base::AutoLock lock(mutex_);
  // First make sure all the lock-free operations are settled.
  // Since insertions_left_till_rehashing_ reached zero all the
  // incoming operations will evenually end up blocked on the mutex.
  while (base::subtle::Acquire_Load(&operations_in_flight)) {}

  map_.insert(address, value);

}

}  // namespace blink

#endif  // LockFreeMap_h
