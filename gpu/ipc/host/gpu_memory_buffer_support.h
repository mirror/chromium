// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef GPU_IPC_HOST_GPU_MEMORY_BUFFER_SUPPORT_H_
#define GPU_IPC_HOST_GPU_MEMORY_BUFFER_SUPPORT_H_

#include <stdint.h>

#include "ui/gfx/buffer_types.h"

namespace gpu {

// Returns the OpenGL target to use for image textures.
uint32_t GetImageTextureTarget(gfx::BufferFormat format,
                               gfx::BufferUsage usage);

}  // namespace gpu

#endif  // GPU_IPC_HOST_GPU_MEMORY_BUFFER_SUPPORT_H_
