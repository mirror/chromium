// Copyright (c) 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "gpu/command_buffer/client/client_transfer_cache.h"

namespace gpu {

ClientTransferCache::ClientTransferCache(gles2::GLES2CmdHelper* helper,
                                         MappedMemoryManager* manager)
    : helper_(helper), manager_(manager) {}

TransferCacheEntryId ClientTransferCache::CreateCacheEntry(
    std::unique_ptr<ClientTransferCacheEntry> entry) {
  size_t size = entry->DataSize();
  ScopedMappedMemoryPtr mapped_alloc(size, helper_, manager_);
  DCHECK(mapped_alloc.valid());
  bool succeeded = entry->Serialize(
      size, reinterpret_cast<uint8_t*>(mapped_alloc.address()));
  DCHECK(succeeded);
  TransferCacheEntryId id = discardable_manager_->CreateHandle(helper_);
  helper_->CreateTransferCacheEntryCHROMIUM(
      id, static_cast<GLuint>(entry->Type()), mapped_alloc.shm_id(),
      mapped_alloc.offset());
  return id;
}

bool ClientTransferCache::LockTransferCacheEntry(TransferCacheEntryId id) {
  return discardable_manager_->LockHandle(id);
}

void ClientTransferCache::UnlockTransferCacheEntry(TransferCacheEntryId id) {
  helper_->UnlockTransferCacheEntryCHROMIUM(id);
}

void ClientTransferCache::DeleteTransferCacheEntry(TransferCacheEntryId id) {
  discardable_manager_->FreeHandle(id);
  helper_->DeleteTransferCacheEntryCHROMIUM(id);
}

}  // namespace gpu
