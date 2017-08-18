// Copyright (c) 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef GPU_COMMAND_BUFFER_SERVICE_SERVICE_UTILS_H_
#define GPU_COMMAND_BUFFER_SERVICE_SERVICE_UTILS_H_

#include "gpu/gpu_export.h"
#include "ui/gl/gl_context.h"

namespace gpu {
struct GpuPreferences;

namespace gles2 {
struct ContextCreationAttribHelper;
class ContextGroup;

GPU_EXPORT gl::GLContextAttribs GenerateGLContextAttribs(
    const ContextCreationAttribHelper& attribs_helper,
    const ContextGroup* context_group);

// Checks to see if passthrough command decoders should be created by checing if
// they are requested via GpuPreferences and are supported by driver
GPU_EXPORT bool CreatePassthroughCommandDecoders(
    const GpuPreferences& gpu_preferences);

}  // namespace gles2
}  // namespace gpu

#endif  // GPU_COMMAND_BUFFER_SERVICE_SERVICE_UTILS_H_
