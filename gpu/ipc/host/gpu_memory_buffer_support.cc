// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "gpu/ipc/host/gpu_memory_buffer_support.h"

#include "base/logging.h"
#include "base/stl_util.h"
#include "build/build_config.h"
#include "gpu/command_buffer/common/gpu_memory_buffer_support.h"
#include "gpu/ipc/common/gpu_memory_buffer_support.h"
#include "ui/gl/gl_bindings.h"
#include "ui/gl/gl_implementation.h"

namespace gpu {

bool GetImageNeedsPlatformSpecificTextureTarget(gfx::BufferFormat format,
                                                gfx::BufferUsage usage) {
#if defined(USE_OZONE) || defined(OS_MACOSX) || defined(OS_WIN) || \
    defined(OS_ANDROID)
  return base::ContainsKey(GetNativeGpuMemoryBufferConfigurations(),
                           std::make_pair(format, usage));
#else  // defined(USE_OZONE) || defined(OS_MACOSX)
  return false;
#endif
}
}  // namespace gpu
