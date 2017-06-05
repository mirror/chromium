// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/threading/sequence_local_storage.h"

#include "base/atomic_sequence_num.h"
#include "base/lazy_instance.h"
#include "base/threading/thread_local.h"

namespace base {
namespace internal {

namespace {
StaticAtomicSequenceNumber g_sequence_local_storage_slot_generator;

LazyInstance<ThreadLocalPointer<SequenceLocalStorage>>::Leaky
    tls_current_sequence_local_storage = LAZY_INSTANCE_INITIALIZER;
}  // namespace

SequenceLocalStorage::SequenceLocalStorage() = default;

SequenceLocalStorage::~SequenceLocalStorage() {
  for (auto& entry : sls_map) {
    ValDtorPair& val_dtor_pair = entry.second;

    void (*dtor)(void*) = val_dtor_pair.dtor;
    void* val = val_dtor_pair.val;

    dtor(val);
  }
}

ScopedSetSequenceLocalStorageForCurrentThread::
    ScopedSetSequenceLocalStorageForCurrentThread(
        SequenceLocalStorage* sequence_local_storage)
    : sequence_local_storage_(sequence_local_storage) {
  DCHECK(!tls_current_sequence_local_storage.Get().Get());
  tls_current_sequence_local_storage.Get().Set(sequence_local_storage_);
}

ScopedSetSequenceLocalStorageForCurrentThread::
    ~ScopedSetSequenceLocalStorageForCurrentThread() {
  DCHECK_EQ(tls_current_sequence_local_storage.Get().Get(),
            sequence_local_storage_);

  tls_current_sequence_local_storage.Get().Set(nullptr);
}

int SequenceLocalStorage::GetNextSequenceLocalStorageSlotNumber() {
  return g_sequence_local_storage_slot_generator.GetNext();
}

SequenceLocalStorage& SequenceLocalStorage::GetForCurrentThread() {
  SequenceLocalStorage* current_sequence_local_storage =
      tls_current_sequence_local_storage.Get().Get();

  DCHECK(current_sequence_local_storage);

  return *current_sequence_local_storage;
}
}  // namespace internal
}  // namespace base
