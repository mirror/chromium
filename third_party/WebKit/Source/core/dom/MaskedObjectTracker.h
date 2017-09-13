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

// TODO(jbroman): Write a nice, detailed comment.
template <typename T>
class MaskedObjectTracker {
  DISALLOW_NEW();
  WTF_MAKE_NONCOPYABLE(MaskedObjectTracker);

  struct Entry {
    T* object;
    unsigned mask;
    DEFINE_INLINE_TRACE() {}
    bool operator==(const Entry& other) {
      return object == other.object && mask == other.mask;
    }
  };

 public:
  MaskedObjectTracker() {}

  void Add(T* object, unsigned mask) {
    Entry entry = {object, mask};
    DCHECK(std::find(data_.begin(), data_.end(), entry) == data_.end());
    data_.push_back(entry);
    data_.ShrinkToReasonableCapacity();
    mask_ |= mask;
  }

  void Remove(T* object, unsigned mask) {
    auto it = std::find(data_.begin(), data_.end(), Entry{object, mask});
    DCHECK(it != data_.end());
    data_.erase(it - data_.begin());
    data_.ShrinkToReasonableCapacity();
    RecomputeMask();
  }

  bool MatchesMask(unsigned query) const { return mask_ & query; }

  DEFINE_INLINE_TRACE() {
    visitor->Trace(data_);
    visitor->RegisterWeakCallback(this, [](Visitor*, void* self) {
      auto& data = static_cast<MaskedObjectTracker*>(self)->data_;
      auto it = std::remove_if(data.begin(), data.end(), [](Entry entry) {
        return !ObjectAliveTrait<T>::IsHeapObjectAlive(entry.object);
      });
      if (it == data.end())
        return;
      // WTF::Vector promises not to actually reallocate the buffer in this
      // case, so this is safe even though we cannot do allocation during a weak
      // callback.
      data.Shrink(it - data.begin());
      static_cast<MaskedObjectTracker*>(self)->RecomputeMask();
    });
  }

 private:
  void RecomputeMask() {
    unsigned mask = 0;
    for (const auto& entry : data_)
      mask |= entry.mask;
    mask_ = mask;
  }

  HeapVector<Entry> data_;
  unsigned mask_ = 0;
};

}  // namespace blink

#endif  // MaskedObjectTracker_h
