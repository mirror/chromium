// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CRAZY_LINKER_POINTER_SET_H
#define CRAZY_LINKER_POINTER_SET_H

#include "crazy_linker_util.h"

namespace crazy {

// A small container for pointer values (addresses).
class PointerSet {
 public:
  PointerSet() = default;

  // Return true iff the set is empty.
  inline bool IsEmpty() const { return items_.IsEmpty(); }

  // Return the number of items in the set.
  inline size_t GetCount() const { return items_.GetCount(); }

  void* operator[](size_t pos) const { return items_[pos]; }

  // Add a new value to the set.
  bool Add(void* item);

  // Remove value |item| from the set, if needed. Returns true if the value
  // was previously in the set, false otherwise.
  bool Remove(void* item);

  // Returns true iff the set contains |item|, false otherwise.
  bool Has(void* item) const;

  // Return a copy of the values in the set into |*out|. Useful for testing.
  void GetValuesForTesting(Vector<void*>* out) const;

 private:
  // TECHNICAL NOTE: The current implementation uses a simple sorted array,
  // and thus should perform well for sets of a few hundred items, when
  // insertions and removals are pretty rare, but lookups need to be fast.

  Vector<void*> items_;
};

}  // namespace crazy

#endif  // CRAZY_LINKER_POINTER_SET_H
