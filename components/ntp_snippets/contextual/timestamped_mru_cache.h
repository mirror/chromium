// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_NTP_SNIPPETS_CONTEXTUAL_TIMESTAMPED_MRU_CACHE_H_
#define COMPONENTS_NTP_SNIPPETS_CONTEXTUAL_TIMESTAMPED_MRU_CACHE_H_

#include "base/containers/mru_cache.h"
#include "base/optional.h"
#include "base/time/time.h"

namespace ntp_snippets {

namespace internal {

// TODO(gaschler): Add unit test to verify caching and max_age behavior.
// This non-persistent cache guarantees to hold no more than max_capacity
// items and returns only items younger than max_age.
template <class KeyType, class PayloadType>
class TimestampedMRUCache {
 public:
  TimestampedMRUCache(size_t max_capacity, base::TimeDelta max_age)
      : cache_(max_capacity), max_age_(max_age) {}

  ~TimestampedMRUCache() = default;

  // Returns base::nullopt if cache has no response younger than max_age.
  base::Optional<PayloadType> Get(const KeyType& key) {
    auto found_iterator = cache_.Get(key);
    if (found_iterator == cache_.end()) {
      return base::nullopt;
    }

    const PayloadType& value = found_iterator->second.data;
    const base::Time& timestamp = found_iterator->second.time;
    if (timestamp + max_age_ < base::Time::Now()) {
      // Cached element is too old, discard it.
      cache_.Erase(found_iterator);
      return base::nullopt;
    }

    return value;
  }

  // Adds an item to the cache with the current timestamp.
  void Put(const KeyType& key, const PayloadType& value) {
    cache_.Put(key, TimestampedData<PayloadType>{value, base::Time::Now()});
  }

 private:
  template <class DataType>
  struct TimestampedData {
   public:
    DataType data;
    base::Time time;
  };

  base::MRUCache<KeyType, TimestampedData<PayloadType>> cache_;

  base::TimeDelta max_age_;

  DISALLOW_COPY_AND_ASSIGN(TimestampedMRUCache);
};

}  // namespace internal

}  // namespace ntp_snippets

#endif  // COMPONENTS_NTP_SNIPPETS_CONTEXTUAL_TIMESTAMPED_MRU_CACHE_H_
