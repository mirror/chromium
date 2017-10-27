// Copyright (c) 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef GPU_COMMAND_BUFFER_CLIENT_CLIENT_DISCARDABLE_TEXTURE_MANAGER_H_
#define GPU_COMMAND_BUFFER_CLIENT_CLIENT_DISCARDABLE_TEXTURE_MANAGER_H_

#include <map>

#include "gpu/command_buffer/client/client_discardable_manager.h"

namespace gpu {

// A helper class used to manage discardable textures. Makes use of
// ClientDiscardableManager. When the GLES2 impl is done with a texture (the
// texture is being deleted), it should call FreeTexture to allow helper memory
// to be reclaimed.
class GPU_EXPORT ClientDiscardableTextureManager {
 public:
  ClientDiscardableTextureManager(
      ClientDiscardableManager* discardable_manager);
  ~ClientDiscardableTextureManager();
  DiscardableHandleId InitializeTexture(gles2::GLES2CmdHelper* command_buffer,
                                        uint32_t texture_id);
  bool LockTexture(uint32_t texture_id);
  void FreeTexture(uint32_t texture_id);
  bool TextureIsValid(uint32_t texture_id) const;

  // Test only functions.
  void CheckPendingForTesting(CommandBuffer* command_buffer) {
    discardable_manager_->CheckPendingForTesting(command_buffer);
  }
  void SetElementCountForTesting(uint32_t count) {
    discardable_manager_->SetElementCountForTesting(count);
  }
  ClientDiscardableHandle GetHandleForTesting(uint32_t texture_id);

 private:
  std::map<uint32_t, DiscardableHandleId> texture_id_to_handle_id_;
  ClientDiscardableManager* discardable_manager_;
};

}  // namespace gpu

#endif  // GPU_COMMAND_BUFFER_CLIENT_CLIENT_DISCARDABLE_TEXTURE_MANAGER_H_
