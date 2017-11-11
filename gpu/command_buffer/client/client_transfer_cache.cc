// Copyright (c) 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "gpu/command_buffer/client/client_transfer_cache.h"

namespace gpu {

ClientTransferCache::ClientTransferCache() = default;
ClientTransferCache::~ClientTransferCache() = default;

cc::TransferCacheEntryId ClientTransferCache::CreateCacheEntry(
    gles2::GLES2Interface* gl,
    CommandBuffer* command_buffer,
    const cc::ClientTransferCacheEntry& entry) {
  cc::TransferCacheEntryId id =
      discardable_manager_.CreateHandle(command_buffer);
  ClientDiscardableHandle handle = discardable_manager_.GetHandle(id);
  gl->CreateTransferCacheEntryCHROMIUM(id.GetUnsafeValue(), handle.shm_id(),
                                       handle.byte_offset(), entry);
  return id;
}

bool ClientTransferCache::LockTransferCacheEntry(cc::TransferCacheEntryId id) {
  return discardable_manager_.LockHandle(id);
}

void ClientTransferCache::UnlockTransferCacheEntry(
    gles2::GLES2Interface* gl,
    cc::TransferCacheEntryId id) {
  gl->UnlockTransferCacheEntryCHROMIUM(id.GetUnsafeValue());
}

void ClientTransferCache::DeleteTransferCacheEntry(
    gles2::GLES2Interface* gl,
    cc::TransferCacheEntryId id) {
  discardable_manager_.FreeHandle(id);
  gl->DeleteTransferCacheEntryCHROMIUM(id.GetUnsafeValue());
}

}  // namespace gpu
