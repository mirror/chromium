// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef BASE_THREADING_SEQUENCE_LOCAL_STORAGE_H_
#define BASE_THREADING_SEQUENCE_LOCAL_STORAGE_H_

#include <memory>
#include <utility>

#include "base/containers/flat_map.h"

namespace base {
namespace internal {

struct ValDtorPair {
  void* val = nullptr;
  void (*dtor)(void*);
};

class BASE_EXPORT SequenceLocalStorage {
 public:
  template <typename T, typename Deleter = std::default_delete<T>>
  class Slot {
   public:
    Slot() : slot_id(GetNextSequenceLocalStorageSlotNumber()) {}
    ~Slot() = default;

    // Get the sequence-local value stored in this slot.
    T& Get() {
      base::flat_map<int, ValDtorPair>& sls_map =
          SequenceLocalStorage::GetForCurrentThread().sls_map;

      return *(static_cast<T*>(sls_map[slot_id].val));
    }

    // Set this slot's sequence-local value to |value|.
    void Set(T value) {
      base::flat_map<int, ValDtorPair>& sls_map =
          SequenceLocalStorage::GetForCurrentThread().sls_map;

      ValDtorPair val_dtor_pair;
      val_dtor_pair.val = new T(std::move<T>(value));
      val_dtor_pair.dtor = Deleter::operator();

      sls_map[slot_id] = val_dtor_pair;
    }

   private:
    const int slot_id;
    DISALLOW_COPY_AND_ASSIGN(Slot);
  };

  static int GetNextSequenceLocalStorageSlotNumber();

  static SequenceLocalStorage& GetForCurrentThread();

  SequenceLocalStorage();
  ~SequenceLocalStorage();

 private:
  base::flat_map<int, ValDtorPair> sls_map;  // slot_id to ValDtorPair map
  DISALLOW_COPY_AND_ASSIGN(SequenceLocalStorage);
};

class BASE_EXPORT ScopedSetSequenceLocalStorageForCurrentThread {
 public:
  ScopedSetSequenceLocalStorageForCurrentThread(
      SequenceLocalStorage* sequence_local_storage);

  ~ScopedSetSequenceLocalStorageForCurrentThread();

 private:
  SequenceLocalStorage* sequence_local_storage_;
  DISALLOW_COPY_AND_ASSIGN(ScopedSetSequenceLocalStorageForCurrentThread);
};
}  // namespace internal
}  // namespace base

#endif
