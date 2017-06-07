// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef BASE_THREADING_SEQUENCE_LOCAL_STORAGE_SLOT_H_
#define BASE_THREADING_SEQUENCE_LOCAL_STORAGE_SLOT_H_

#include <memory>
#include <utility>

#include "base/threading/sequence_local_storage_map.h"

namespace base {

namespace internal {
BASE_EXPORT int GetNextSequenceLocalStorageSlotNumber();
}  // namespace internal

// SequenceLocalStorageSlot allows arbitrary values to be stored and retrieved
// from a Sequence. SequenceLocalStorageSlot must be used within the scope of
// a ScopedSetSequenceLocalStorageMapForCurrentThread object.
template <typename T, typename Deleter = std::default_delete<T>>
class SequenceLocalStorageSlot {
 public:
  SequenceLocalStorageSlot()
      : slot_id_(internal::GetNextSequenceLocalStorageSlotNumber()) {}
  ~SequenceLocalStorageSlot() = default;

  // Get the sequence-local value stored in this slot.
  // TODO(jeffreyhe): Change this method to return a default-constructed value
  // instead of crashing if no value was previously set.
  // https://crbug.com/695727
  T& Get() {
    void* value =
        internal::SequenceLocalStorageMap::GetForCurrentThread().Get(slot_id_);
    return *(static_cast<T*>(value));
  }

  // Set this slot's sequence-local value to |value|.
  // Note that if size(T) is big, it may be more appropriate to instead store a
  // std::unique_ptr<T> to avoid a costly copy.
  void Set(T value) {
    internal::SequenceLocalStorageMap::ValueDestructorPair
        value_destructor_pair;

    // Allocates the |value| with new.
    // This is done with new rather than MakeUnique because
    // SequenceLocalStorageMap must store void* because it needs to be able to
    // store arbitrary values. Storing and then destroying a unique_ptr<void>
    //  would not call the destructor for the actual type of the value.
    // The value will be destructed either when the value is overwritten by
    // another call to SequenceLocalStorageMap::Set or when the
    // SequenceLocalStorageMap is destructed.
    value_destructor_pair.value = new T(std::move(value));

    value_destructor_pair.destructor = [](void* ptr) {
      Deleter()(static_cast<T*>(ptr));
    };

    internal::SequenceLocalStorageMap::GetForCurrentThread().Set(
        slot_id_, value_destructor_pair);
  }

 private:
  const int slot_id_;
  DISALLOW_COPY_AND_ASSIGN(SequenceLocalStorageSlot);
};

}  // namespace base
#endif
