// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "gpu/ipc/service/gpu_memory_buffer_factory_android_hardware_buffer.h"

#include "base/logging.h"
#include "base/memory/shared_memory_handle.h"
#include "ui/gl/gl_image_egl.h"

#include "base/threading/platform_thread.h"

#include "base/memory/ptr_util.h"

#include "build/build_config.h"

namespace gpu {

GpuMemoryBufferFactoryAndroidHardwareBuffer::
    GpuMemoryBufferFactoryAndroidHardwareBuffer() {}

GpuMemoryBufferFactoryAndroidHardwareBuffer::
    ~GpuMemoryBufferFactoryAndroidHardwareBuffer() {}

gfx::GpuMemoryBufferHandle
GpuMemoryBufferFactoryAndroidHardwareBuffer::CreateGpuMemoryBuffer(
    gfx::GpuMemoryBufferId id,
    const gfx::Size& size,
    gfx::BufferFormat format,
    gfx::BufferUsage usage,
    int client_id,
    SurfaceHandle surface_handle) {
  NOTIMPLEMENTED();
  return gfx::GpuMemoryBufferHandle();
}

void GpuMemoryBufferFactoryAndroidHardwareBuffer::DestroyGpuMemoryBuffer(
    gfx::GpuMemoryBufferId id,
    int client_id) {}

ImageFactory* GpuMemoryBufferFactoryAndroidHardwareBuffer::AsImageFactory() {
  return this;
}

scoped_refptr<gl::GLImage>
GpuMemoryBufferFactoryAndroidHardwareBuffer::CreateImageForGpuMemoryBuffer(
    const gfx::GpuMemoryBufferHandle& handle,
    const gfx::Size& size,
    gfx::BufferFormat format,
    unsigned internalformat,
    int client_id,
    SurfaceHandle surface_handle) {
  // TRACE_EVENT0("gpu", "GpuMemoryBufferFactoryAndroidHardwareBuffer::Create");

  CHECK_EQ(handle.type, gfx::ANDROID_HARDWARE_BUFFER);

  AHardwareBuffer* buffer = handle.handle.GetMemoryObject();
  if (!buffer)
    return nullptr;

  EGLint attribs[] = {EGL_IMAGE_PRESERVED_KHR, EGL_FALSE, EGL_NONE};

  scoped_refptr<gl::GLImageEGL> image(new gl::GLImageEGL(size));
  EGLClientBuffer clientBuffer = eglGetNativeClientBufferANDROID(buffer);
  if (!image->Initialize(EGL_NATIVE_BUFFER_ANDROID, clientBuffer, attribs)) {
    LOG(ERROR) << "Failed to create GLImage " << size.ToString();
    return nullptr;
  }
  return image;
}

scoped_refptr<gl::GLImage>
GpuMemoryBufferFactoryAndroidHardwareBuffer::CreateAnonymousImage(
    const gfx::Size& size,
    gfx::BufferFormat format,
    gfx::BufferUsage usage,
    unsigned internalformat) {
  NOTIMPLEMENTED();
  return nullptr;
}

unsigned GpuMemoryBufferFactoryAndroidHardwareBuffer::RequiredTextureType() {
  return GL_TEXTURE_EXTERNAL_OES;
}

}  // namespace gpu
