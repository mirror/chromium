// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/test/sync/null_directory_change_delegate.h"

namespace syncable {

NullDirectoryChangeDelegate::~NullDirectoryChangeDelegate() {}

void NullDirectoryChangeDelegate::HandleCalculateChangesChangeEventFromSyncApi(
    const EntryKernelMutationSet& mutations,
    BaseTransaction* trans) {}

void NullDirectoryChangeDelegate::HandleCalculateChangesChangeEventFromSyncer(
    const EntryKernelMutationSet& mutations,
    BaseTransaction* trans) {}

ModelTypeBitSet NullDirectoryChangeDelegate::HandleTransactionEndingChangeEvent(
      BaseTransaction* trans) {
  return ModelTypeBitSet();
}

void NullDirectoryChangeDelegate::HandleTransactionCompleteChangeEvent(
    const ModelTypeBitSet& models_with_changes) {}

}  // namespace syncable
