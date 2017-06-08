// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/threading/sequence_local_storage_map.h"

#include <utility>

#include "base/lazy_instance.h"
#include "base/logging.h"
#include "base/threading/thread_local.h"

namespace base {
namespace internal {

namespace {
LazyInstance<ThreadLocalPointer<SequenceLocalStorageMap>>::Leaky
    tls_current_sequence_local_storage = LAZY_INSTANCE_INITIALIZER;
}  // namespace

SequenceLocalStorageMap::SequenceLocalStorageMap() = default;

SequenceLocalStorageMap::~SequenceLocalStorageMap() = default;

ScopedSetSequenceLocalStorageMapForCurrentThread::
    ScopedSetSequenceLocalStorageMapForCurrentThread(
        SequenceLocalStorageMap* sequence_local_storage) {
  DCHECK(!tls_current_sequence_local_storage.Get().Get());
  tls_current_sequence_local_storage.Get().Set(sequence_local_storage);
}

ScopedSetSequenceLocalStorageMapForCurrentThread::
    ~ScopedSetSequenceLocalStorageMapForCurrentThread() {
  tls_current_sequence_local_storage.Get().Set(nullptr);
}

SequenceLocalStorageMap& SequenceLocalStorageMap::GetForCurrentThread() {
  SequenceLocalStorageMap* current_sequence_local_storage =
      tls_current_sequence_local_storage.Get().Get();

  DCHECK(current_sequence_local_storage)
      << "SequenceLocalStorageSlot cannot be used because no "
         "SequenceLocalStorageMap was stored in TLS. Use "
         "ScopedSetSequenceLocalStorageMapForCurrentThread to store a "
         "SequenceLocalStorageMap object in TLS.";

  return *current_sequence_local_storage;
}

void* SequenceLocalStorageMap::Get(int slot_id) {
  const auto it = sls_map.find(slot_id);
  if (it == sls_map.end())
    return nullptr;
  return it->second.value();
}

void SequenceLocalStorageMap::Set(
    int slot_id,
    SequenceLocalStorageMap::ValueDestructorPair value_destructor_pair) {
  auto it = sls_map.find(slot_id);

  if (it == sls_map.end())
    sls_map.emplace(slot_id, std::move(value_destructor_pair));
  else
    it->second = std::move(value_destructor_pair);
}

}  // namespace internal
}  // namespace base
