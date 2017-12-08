// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "gpu/ipc/host/gpu_memory_buffer_support.h"

#include "base/logging.h"
#include "build/build_config.h"
#include "gpu/ipc/common/gpu_memory_buffer_support.h"
#include "gpu/ipc/common/gpu_switches.h"
#include "ui/gl/gl_bindings.h"
#include "ui/gl/gl_implementation.h"

namespace gpu {

uint32_t GetImageTextureTarget(gfx::BufferFormat format,
                               gfx::BufferUsage usage) {
#if defined(USE_OZONE) || defined(OS_MACOSX) || defined(OS_WIN) || \
    defined(OS_ANDROID)
  GpuMemoryBufferConfigurationSet native_configurations =
      GetNativeGpuMemoryBufferConfigurations();
  if (native_configurations.find(std::make_pair(format, usage)) ==
      native_configurations.end()) {
    return GL_TEXTURE_2D;
  }

  switch (GetNativeGpuMemoryBufferType()) {
    case gfx::NATIVE_PIXMAP:
    case gfx::ANDROID_HARDWARE_BUFFER:
      // GPU memory buffers that are shared with the GL using EGLImages
      // require TEXTURE_EXTERNAL_OES.
      return GL_TEXTURE_EXTERNAL_OES;
    case gfx::IO_SURFACE_BUFFER:
      // IOSurface backed images require GL_TEXTURE_RECTANGLE_ARB.
      return GL_TEXTURE_RECTANGLE_ARB;
    case gfx::SHARED_MEMORY_BUFFER:
    case gfx::EMPTY_BUFFER:
      break;
    case gfx::DXGI_SHARED_HANDLE:
      return GL_TEXTURE_2D;
  }
  NOTREACHED();
  return GL_TEXTURE_2D;
#else  // defined(USE_OZONE) || defined(OS_MACOSX)
  return GL_TEXTURE_2D;
#endif
}

}  // namespace gpu
