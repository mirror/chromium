// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "core/layout/ng/inline/ng_inline_fragment_iterator.h"

#include "core/layout/LayoutObject.h"
#include "core/layout/ng/ng_physical_box_fragment.h"
#include "platform/wtf/HashMap.h"

namespace blink {

namespace {

// Returns a static empty list.
const NGInlineFragmentIterator::Results* EmptyResults() {
  DEFINE_THREAD_SAFE_STATIC_LOCAL(NGInlineFragmentIterator::Results,
                                  empty_reuslts, ());
  return &empty_reuslts;
}

// The expected use of the iterator is to get fragments of all inline child
// LayoutObjects. To avoid O(n^2) in such case, create a map of chlidren and
// keep in a cache per the containing box.
//
// Because the caller is likely traversing the tree, keep only one cache entry.
//
// To sipmlify, the cached map ownership is moved to NGInlineFragmentIterator
// when instantiated, and moved back to the cache when destructed, assuming
// there are no use cases to nest the iterator.
struct CacheEntry {
  RefPtr<const NGPhysicalContainerFragment> container;
  NGInlineFragmentIterator::LayoutObjectMap map;
};

CacheEntry* GetCacheEntry() {
  DEFINE_THREAD_SAFE_STATIC_LOCAL(CacheEntry, cache_entry, ());
  return &cache_entry;
}

bool GetCachedMap(const NGPhysicalContainerFragment& container,
                  NGInlineFragmentIterator::LayoutObjectMap* map) {
  CacheEntry* cache = GetCacheEntry();
  if (cache->container.get() != &container)
    return false;
  cache->map.swap(*map);
  return true;
}

void AddToCache(const NGPhysicalContainerFragment& container,
                NGInlineFragmentIterator::LayoutObjectMap* map) {
  CacheEntry* cache = GetCacheEntry();
  cache->container = &container;
  cache->map.swap(*map);
}

}  // namespace

NGInlineFragmentIterator::NGInlineFragmentIterator(
    const NGPhysicalContainerFragment& container,
    const LayoutObject* filter)
    : container_(container) {
  DCHECK(filter);

  // Check the cache. This iterator owns the map while alive.
  if (!GetCachedMap(container, &map_)) {
    // Create a map if it's not in the cache.
    CollectInlineFragments(container, {}, &map_);
  }

  const auto it = map_.find(filter);
  results_ = it != map_.end() ? &it->value : EmptyResults();
}

NGInlineFragmentIterator::~NGInlineFragmentIterator() {
  // Return the ownership of the map to the cache.
  AddToCache(container_, &map_);
}

// Collect inline child fragments of the container fragment, accumulating the
// offset to the container box.
void NGInlineFragmentIterator::CollectInlineFragments(
    const NGPhysicalContainerFragment& container,
    NGPhysicalOffset offset_to_container_box,
    LayoutObjectMap* map) {
  for (const auto& child : container.Children()) {
    NGPhysicalOffset child_offset = child->Offset() + offset_to_container_box;

    // Add to the map if the fragment has LayoutObject.
    if (const LayoutObject* child_layout_object = child->GetLayoutObject()) {
      const auto it = map->find(child_layout_object);
      if (it == map->end()) {
        map->insert(child_layout_object, Results{{child.get(), child_offset}});
      } else {
        it->value.push_back(Result{child.get(), child_offset});
      }
    }

    // Traverse descendants unless the fragment is laid out separately from the
    // inline layout algorithm.
    if (child->IsContainer() && !child->IsBlockLayoutRoot()) {
      CollectInlineFragments(ToNGPhysicalContainerFragment(*child),
                             child_offset, map);
    }
  }
}

}  // namespace blink
