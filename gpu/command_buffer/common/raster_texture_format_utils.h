// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef GPU_COMMAND_BUFFER_RASTER_TEXTURE_FORMAT_UTILS_H_
#define GPU_COMMAND_BUFFER_RASTER_TEXTURE_FORMAT_UTILS_H_

#include "gpu/command_buffer/common/raster_texture_format.h"
#include "gpu/gpu_export.h"

namespace gpu {
namespace raster {

GPU_EXPORT unsigned int TextureStorageFormat(TextureFormat format);
GPU_EXPORT unsigned int GLDataFormat(TextureFormat format);
GPU_EXPORT unsigned int GLDataType(TextureFormat format);
GPU_EXPORT unsigned int GLInternalFormat(TextureFormat format);

}  // namespace raster
}  // namespace gpu

#endif  // GPU_COMMAND_BUFFER_RASTER_TEXTURE_FORMAT_UTILS_H_
