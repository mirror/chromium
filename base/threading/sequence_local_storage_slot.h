// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef BASE_THREADING_SEQUENCE_LOCAL_STORAGE_SLOT_H_
#define BASE_THREADING_SEQUENCE_LOCAL_STORAGE_SLOT_H_

#include "base/threading/sequence_local_storage.h"

namespace base {

namespace internal {

BASE_EXPORT int GetNextSequenceLocalStorageSlotNumber();

}  // namespace internal

template <typename T, typename Deleter = std::default_delete<T>>
class SequenceLocalStorageSlot {
 public:
  SequenceLocalStorageSlot()
      : slot_id(internal::GetNextSequenceLocalStorageSlotNumber()) {}
  ~SequenceLocalStorageSlot() = default;

  // Get the sequence-local value stored in this slot.
  T& Get() {
    internal::ValDtorPair& val_dtor_pair =
        internal::SequenceLocalStorage::GetForCurrentThread().Get(slot_id);

    return *(static_cast<T*>(val_dtor_pair.val));
  }

  // Set this slot's sequence-local value to |value|.
  void Set(T value) {
    internal::ValDtorPair val_dtor_pair;
    val_dtor_pair.val = new T(std::move(value));

    val_dtor_pair.dtor = [](void* ptr) {
      Deleter deleter;
      deleter(static_cast<T*>(ptr));
    };

    internal::SequenceLocalStorage::GetForCurrentThread().Set(slot_id,
                                                              val_dtor_pair);
  }

 private:
  const int slot_id;
  DISALLOW_COPY_AND_ASSIGN(SequenceLocalStorageSlot);
};

}  // namespace base
#endif
