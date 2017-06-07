// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef BASE_THREADING_SEQUENCE_LOCAL_STORAGE_H_
#define BASE_THREADING_SEQUENCE_LOCAL_STORAGE_H_

#include "base/base_export.h"
#include "base/containers/flat_map.h"

namespace base {
namespace internal {

// SequenceLocalStorageMap is a wrapper to a map and is set in TLS by
// ScopedSetSequenceMapLocalStorageForCurrentThread. Each Sequence has its own
// SequenceLocalStorageMap so ScopedSetSequenceMapLocalStorageForCurrentThread
// should be used to store the SequenceLocalStorageMap for the current sequence.
// The map is used as storage for SequenceLocalStorageSlot to store and retrieve
// values (and their destructors) to and from a sequence.
// Destroying a SequenceLocalStorageMap object invokes the destructor of every
// stored value.
class BASE_EXPORT SequenceLocalStorageMap {
 public:
  SequenceLocalStorageMap();
  ~SequenceLocalStorageMap();

  // Returns the SequenceLocalStorage bound to the current thread. It is invalid
  // to call this outside the scope of a
  // ScopedSetSequenceLocalStorageForCurrentThread.
  static SequenceLocalStorageMap& GetForCurrentThread();

  struct ValueDestructorPair {
    void* value = nullptr;
    void (*destructor)(void*);
  };

  // Returns the value stored in |slot_id|, or nullptr if no value was stored.
  void* Get(int slot_id);

  // Stores |value_destructor_pair| in |slot_id|. Overwrites any previously
  // stored value.
  void Set(int slot_id, ValueDestructorPair value_destructor_pair);

 private:
  // Map from slot id to ValueDestructorPair.
  base::flat_map<int, ValueDestructorPair> sls_map;

  DISALLOW_COPY_AND_ASSIGN(SequenceLocalStorageMap);
};

// Within the scope of this object,
// SequenceLocalStorageMap::GetForCurrentThread() will return a reference to
// the SequenceLocalStorageMap object passed to the constructor.
// It is invalid to instantiate an instance of this while already being
// within the scope of another ScopedSetSequenceLocalStorageMapForCurrentThread
// object.
class BASE_EXPORT ScopedSetSequenceLocalStorageMapForCurrentThread {
 public:
  ScopedSetSequenceLocalStorageMapForCurrentThread(
      SequenceLocalStorageMap* sequence_local_storage);

  ~ScopedSetSequenceLocalStorageMapForCurrentThread();

 private:
  DISALLOW_COPY_AND_ASSIGN(ScopedSetSequenceLocalStorageMapForCurrentThread);
};
}  // namespace internal
}  // namespace base

#endif
