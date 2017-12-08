// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef GPU_CONFIG_GPU_MEMORY_BUFFER_SUPPORT_H_
#define GPU_CONFIG_GPU_MEMORY_BUFFER_SUPPORT_H_

#include "base/containers/hash_tables.h"
#include "base/hash.h"
#include "gpu/gpu_export.h"
#include "ui/gfx/buffer_types.h"
#include "ui/gfx/gpu_memory_buffer.h"

namespace gpu {

using GpuMemoryBufferConfigurationKey =
    std::pair<gfx::BufferFormat, gfx::BufferUsage>;
using GpuMemoryBufferConfigurationSet =
    base::hash_set<GpuMemoryBufferConfigurationKey>;

}  // namespace gpu

namespace BASE_HASH_NAMESPACE {

template <>
struct hash<gpu::GpuMemoryBufferConfigurationKey> {
  size_t operator()(const gpu::GpuMemoryBufferConfigurationKey& key) const {
    return base::HashInts(static_cast<int>(key.first),
                          static_cast<int>(key.second));
  }
};

}  // namespace BASE_HASH_NAMESPACE

namespace gpu {

GPU_EXPORT bool AreNativeGpuMemoryBuffersEnabled();

// Returns the set of supported configurations.
GPU_EXPORT GpuMemoryBufferConfigurationSet
GetNativeGpuMemoryBufferConfigurations();

// Returns true of the OpenGL target to use for the combination of format/usage
// is not GL_TEXTURE_2D but a platform specific texture target.
GPU_EXPORT bool GetImageNeedsPlatformSpecificTextureTarget(
    gfx::BufferFormat format,
    gfx::BufferUsage usage);

// Returns the native GPU memory buffer factory type. Returns EMPTY_BUFFER
// type if native buffers are not supported.
GPU_EXPORT gfx::GpuMemoryBufferType GetNativeGpuMemoryBufferType();

// Returns whether the provided buffer format is supported.
GPU_EXPORT bool IsNativeGpuMemoryBufferConfigurationSupported(
    gfx::BufferFormat format,
    gfx::BufferUsage usage);

}  // namespace gpu

#endif  // GPU_CONFIG_GPU_MEMORY_BUFFER_SUPPORT_H_
