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
  SequenceLocalStorage();
  ~SequenceLocalStorage();

  static SequenceLocalStorage& GetForCurrentThread();

  ValDtorPair& Get(int slot_id);
  void Set(int slot_id, ValDtorPair val_dtor_pair);

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
