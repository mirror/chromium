// Copyright (c) 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_RENDERER_MEDIA_WEBRTC_TWO_KEYS_MAP_H_
#define CONTENT_RENDERER_MEDIA_WEBRTC_TWO_KEYS_MAP_H_

#include <map>
#include <memory>
#include <utility>

#include "base/logging.h"

namespace content {

// A map with up to two keys per entry. An element is inserted with a key, this
// is the primary key. A secondary key can optionally be set to the same entry
// and it may be set at a later point in time than the element was inserted. For
// lookup and erasure both keys can be used.
template <typename PrimaryKey, typename SecondaryKey, typename Value>
class TwoKeysMap {
 public:
  void Insert(PrimaryKey primary, Value value) {
    DCHECK(entries_by_primary_.find(primary) == entries_by_primary_.end());
    auto it = entries_by_primary_
                  .insert(std::make_pair(
                      std::move(primary),
                      std::unique_ptr<Entry>(new Entry(std::move(value)))))
                  .first;
    it->second->primary_it = it;
  }

  void SetSecondaryKey(const PrimaryKey& primary, SecondaryKey secondary) {
    auto it = entries_by_primary_.find(primary);
    DCHECK(it != entries_by_primary_.end());
    DCHECK(entries_by_secondary_.find(secondary) ==
           entries_by_secondary_.end());
    Entry* entry = it->second.get();
    entry->secondary_it =
        entries_by_secondary_
            .insert(std::make_pair(std::move(secondary), entry))
            .first;
  }

  Value* FindByPrimary(const PrimaryKey& primary) const {
    auto it = entries_by_primary_.find(primary);
    if (it == entries_by_primary_.end())
      return nullptr;
    return &it->second->value;
  }

  Value* FindBySecondary(const SecondaryKey& secondary) const {
    auto it = entries_by_secondary_.find(secondary);
    if (it == entries_by_secondary_.end())
      return nullptr;
    return &it->second->value;
  }

  bool EraseByPrimary(const PrimaryKey& primary) {
    auto primary_it = entries_by_primary_.find(primary);
    if (primary_it == entries_by_primary_.end())
      return false;
    if (primary_it->second->secondary_it != entries_by_secondary_.end())
      entries_by_secondary_.erase(primary_it->second->secondary_it);
    entries_by_primary_.erase(primary_it);
    return true;
  }

  bool EraseBySecondary(const SecondaryKey& secondary) {
    auto secondary_it = entries_by_secondary_.find(secondary);
    if (secondary_it == entries_by_secondary_.end())
      return false;
    entries_by_primary_.erase(secondary_it->second->primary_it);
    entries_by_secondary_.erase(secondary_it);
    return true;
  }

  size_t PrimarySize() const { return entries_by_primary_.size(); }
  size_t SecondarySize() const { return entries_by_secondary_.size(); }
  bool empty() const { return entries_by_primary_.empty(); }

 private:
  struct Entry {
    Entry(Value value) : value(std::move(value)) {}

    Value value;
    typename std::map<PrimaryKey, std::unique_ptr<Entry>>::iterator primary_it;
    typename std::map<SecondaryKey, Entry*>::iterator secondary_it;
  };

  typename std::map<PrimaryKey, std::unique_ptr<Entry>> entries_by_primary_;
  typename std::map<SecondaryKey, Entry*> entries_by_secondary_;
};

}  // namespace content

#endif  // CONTENT_RENDERER_MEDIA_WEBRTC_TWO_KEYS_MAP_H_
