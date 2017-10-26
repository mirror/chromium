// Copyright (c) 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "gpu/command_buffer/client/client_discardable_texture_manager.h"

namespace gpu {

ClientDiscardableTextureManager::ClientDiscardableTextureManager(
    ClientDiscardableManager* discardable_manager)
    : discardable_manager_(discardable_manager) {}

ClientDiscardableTextureManager::~ClientDiscardableTextureManager() = default;

DiscardableHandleId ClientDiscardableTextureManager::InitializeTexture(
    gles2::GLES2CmdHelper* command_helper,
    uint32_t texture_id) {
  DCHECK(texture_id_to_handle_id_.find(texture_id) ==
         texture_id_to_handle_id_.end());
  DiscardableHandleId handle_id =
      discardable_manager_->CreateHandle(command_helper);
  texture_id_to_handle_id_[texture_id] = handle_id;
  return handle_id;
}

bool ClientDiscardableTextureManager::LockTexture(uint32_t texture_id) {
  auto found = texture_id_to_handle_id_.find(texture_id);
  DCHECK(found != texture_id_to_handle_id_.end());
  return discardable_manager_->LockHandle(found->second);
}

void ClientDiscardableTextureManager::FreeTexture(uint32_t texture_id) {
  auto found = texture_id_to_handle_id_.find(texture_id);
  if (found == texture_id_to_handle_id_.end())
    return;
  DiscardableHandleId discardable_id = found->second;
  texture_id_to_handle_id_.erase(found);
  return discardable_manager_->FreeHandle(discardable_id);
}

bool ClientDiscardableTextureManager::TextureIsValid(
    uint32_t texture_id) const {
  return texture_id_to_handle_id_.find(texture_id) !=
         texture_id_to_handle_id_.end();
}

ClientDiscardableHandle ClientDiscardableTextureManager::GetHandleForTesting(
    uint32_t texture_id) {
  auto found = texture_id_to_handle_id_.find(texture_id);
  DCHECK(found != texture_id_to_handle_id_.end());
  return discardable_manager_->GetHandleForTesting(found->second);
}

}  // namespace gpu
