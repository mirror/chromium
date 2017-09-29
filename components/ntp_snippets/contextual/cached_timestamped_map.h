// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_NTP_SNIPPETS_CONTEXTUAL_CACHED_TIMESTAMPED_MAP_H_
#define COMPONENTS_NTP_SNIPPETS_CONTEXTUAL_CACHED_TIMESTAMPED_MAP_H_

#include <map>

#include "base/optional.h"
#include "base/time/clock.h"

namespace ntp_snippets {

namespace internal {

// TODO(gaschler): Add unit test to verify caching and max_age behavior.
// Map that guarantees to hold no more than max_capacity items and
// returns only items younger than max_age.
// It can be used for non-persistent caching.
template <typename TKey, typename TValue>
class CachedTimestampedMap {
 public:
  CachedTimestampedMap(std::size_t max_capacity, base::TimeDelta max_age)
      : max_capacity_(max_capacity), max_age_(max_age) {}

  ~CachedTimestampedMap() = default;

  // Returns base::nullopt if cache has no response younger than max_age.
  base::Optional<TValue> Get(const TKey& key) {
    auto found_iterator = map_.find(key);
    if (found_iterator == map_.end()) {
      return base::nullopt;
    }

    TValue& value = found_iterator->second.first;
    base::Time& timestamp = found_iterator->second.second;
    if (timestamp + max_age_ < base::Time::Now()) {
      // Cached element is too old, discard it.
      map_.erase(found_iterator);
      return base::nullopt;
    }

    return base::make_optional(value);
  }

  // Adds an item to the cache with the current timestamp.
  void Set(const TKey& key, const TValue& value) {
    if (map_.size() > max_capacity_ + 1) {
      ReduceSize();
    }
    map_.emplace(std::make_pair(key, std::make_pair(value, base::Time::Now())));
  }

 private:
  void ReduceSize() {
    // TODO(gaschler): Remove the older half of elements instead.
    map_.clear();
  }

  std::map<TKey, std::pair<TValue, base::Time>> map_;

  std::size_t max_capacity_;

  base::TimeDelta max_age_;

  DISALLOW_COPY_AND_ASSIGN(CachedTimestampedMap);
};

}  // namespace internal

}  // namespace ntp_snippets

#endif  // COMPONENTS_NTP_SNIPPETS_CONTEXTUAL_CACHED_TIMESTAMPED_MAP_H_
