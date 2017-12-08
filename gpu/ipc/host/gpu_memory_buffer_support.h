// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef GPU_IPC_HOST_GPU_MEMORY_BUFFER_SUPPORT_H_
#define GPU_IPC_HOST_GPU_MEMORY_BUFFER_SUPPORT_H_

#include "ui/gfx/buffer_types.h"

namespace gpu {

// Returns true if the OpenGL target to use for the combination of format/usage
// is not GL_TEXTURE_2D but a platform specific texture target.
bool GetImageNeedsPlatformSpecificTextureTarget(gfx::BufferFormat format,
                                                gfx::BufferUsage usage);

}  // namespace gpu

#endif  // GPU_IPC_HOST_GPU_MEMORY_BUFFER_SUPPORT_H_
