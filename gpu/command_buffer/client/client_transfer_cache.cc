// Copyright (c) 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "gpu/command_buffer/client/client_transfer_cache.h"

namespace gpu {

ClientTransferCache::ClientTransferCache(
    gles2::GLES2Interface* gl,
    gles2::GLES2CmdHelper* helper,
    ClientDiscardableManager* discardable_manager)
    : gl_(gl), helper_(helper), discardable_manager_(discardable_manager) {}

TransferCacheEntryId ClientTransferCache::CreateCacheEntry(
    std::unique_ptr<ClientTransferCacheEntry> entry) {
  std::vector<uint8_t> data(entry->DataSize());
  bool succeeded = entry->Serialize(data.size(), data.data());
  DCHECK(succeeded);
  TransferCacheEntryId id = discardable_manager_->CreateHandle(helper_);
  gl_->CreateTransferCacheEntryCHROMIUM(id, static_cast<GLuint>(entry->Type()),
                                        data.size(), data.data());
  return id;
}

bool ClientTransferCache::LockTransferCacheEntry(TransferCacheEntryId id) {
  return discardable_manager_->LockHandle(id);
}

void ClientTransferCache::UnlockTransferCacheEntry(TransferCacheEntryId id) {
  gl_->UnlockTransferCacheEntryCHROMIUM(id);
}

void ClientTransferCache::DeleteTransferCacheEntry(TransferCacheEntryId id) {
  discardable_manager_->FreeHandle(id);
  gl_->DeleteTransferCacheEntryCHROMIUM(id);
}

}  // namespace gpu
