// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/threading/sequence_local_storage_slot.h"

#include "base/atomic_sequence_num.h"

namespace base {
namespace internal {

namespace {
StaticAtomicSequenceNumber g_sequence_local_storage_slot_generator;

}  // namespace

int GetNextSequenceLocalStorageSlotNumber() {
  return g_sequence_local_storage_slot_generator.GetNext();
}

}  // namespace internal
}  // namespace base
