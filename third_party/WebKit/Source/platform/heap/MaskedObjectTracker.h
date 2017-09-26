// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MaskedObjectTracker_h
#define MaskedObjectTracker_h

#include <algorithm>

#include "platform/heap/Heap.h"
#include "platform/heap/HeapAllocator.h"
#include "platform/wtf/Allocator.h"
#include "platform/wtf/Noncopyable.h"
#include "platform/wtf/Vector.h"

namespace blink {

// Tracks a set of objects and associated integers, intended to be used
// as a bit mask (possibly of just one bit). Pointed-to objects are held
// weakly (and automatically removed when collected by the GC), or can be
// removed manually.
//
// The set can be queried by providing a mask to be compared with the combined
// (via bitwise-or) mask of all of the values. If there is a match (i.e. an
// element exists for which applying the mask yields a non-zero value), then the
// query matches.
//
// Removal is somewhat inefficient, but querying is very efficient and the data
// stucture is quite compact.
//
// It is invalid to add an (object, mask) pair that is already present, or to
// remove one which is not.
template <typename T>
class MaskedObjectTracker {
  DISALLOW_NEW();
  WTF_MAKE_NONCOPYABLE(MaskedObjectTracker);

  using Entry = std::pair<UntracedMember<T>, unsigned>;

 public:
  MaskedObjectTracker() {}

  void Add(T* object, unsigned mask) {
    Entry entry = {object, mask};
    DCHECK(std::find(data_.begin(), data_.end(), entry) == data_.end());
    data_.push_back(entry);
    mask_ |= mask;
  }

  void Remove(T* object, unsigned mask) {
    auto it = std::find(data_.begin(), data_.end(), Entry{object, mask});
    DCHECK(it != data_.end());
    data_.erase(it);
    data_.ShrinkToReasonableCapacity();
    RecomputeMask();
  }

  bool MatchesMask(unsigned query) const { return mask_ & query; }

  DEFINE_INLINE_TRACE() {
    visitor->RegisterWeakMembers<MaskedObjectTracker,
                                 &MaskedObjectTracker::ClearWeakMembers>(this);
  }

 private:
  void RecomputeMask() {
    unsigned mask = 0;
    for (const auto& entry : data_)
      mask |= entry.second;
    mask_ = mask;
  }

  void ClearWeakMembers(Visitor*) {
    auto it = std::remove_if(data_.begin(), data_.end(), [](Entry entry) {
      return !ObjectAliveTrait<T>::IsHeapObjectAlive(entry.first);
    });
    if (it == data_.end())
      return;

    data_.Shrink(it - data_.begin());
    data_.ShrinkToReasonableCapacity();
    RecomputeMask();
  }

  Vector<Entry> data_;
  unsigned mask_ = 0;
};

}  // namespace blink

#endif  // MaskedObjectTracker_h
