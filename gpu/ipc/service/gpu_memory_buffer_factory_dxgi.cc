// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "gpu/ipc/service/gpu_memory_buffer_factory_dxgi.h"

#include <vector>

#include "base/logging.h"
#include "gpu/GLES2/gl2extchromium.h"
#include "ui/gfx/buffer_format_util.h"
#include "ui/gl/gl_angle_util_win.h"
#include "ui/gl/gl_image_dxgi.h"

#include "base/unguessable_token.h"

#include "base/trace_event/trace_event.h"

namespace gpu {

GpuMemoryBufferFactoryDXGI::GpuMemoryBufferFactoryDXGI() {}

GpuMemoryBufferFactoryDXGI::~GpuMemoryBufferFactoryDXGI() {}

gfx::GpuMemoryBufferHandle GpuMemoryBufferFactoryDXGI::CreateGpuMemoryBuffer(
    gfx::GpuMemoryBufferId id,
    const gfx::Size& size,
    gfx::BufferFormat format,
    gfx::BufferUsage usage,
    int client_id,
    SurfaceHandle surface_handle) {
  TRACE_EVENT0("gpu", "GpuMemoryBufferFactoryDXGI::CreateGpuMemoryBuffer");

  gfx::GpuMemoryBufferHandle handle = {};

  base::win::ScopedComPtr<ID3D11Device> d3d11_device =
      gl::QueryD3D11DeviceObjectFromANGLE();

  D3D11_TEXTURE2D_DESC desc = {
      size.width(),
      size.height(),
      1,
      1,
      DXGI_FORMAT_R8G8B8A8_UNORM,
      {1, 0},
      D3D11_USAGE_DEFAULT,
      D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET,
      0,
      D3D11_RESOURCE_MISC_SHARED_NTHANDLE |
          D3D11_RESOURCE_MISC_SHARED_KEYEDMUTEX};

  base::win::ScopedComPtr<ID3D11Texture2D> d3d11_texture;

  if (FAILED(d3d11_device->CreateTexture2D(&desc, nullptr,
                                           d3d11_texture.GetAddressOf())))
    return handle;

  base::win::ScopedComPtr<IDXGIResource1> dxgi_resource;
  if (FAILED(d3d11_texture.CopyTo(dxgi_resource.GetAddressOf())))
    return handle;

  HANDLE texture_handle;
  if (FAILED(dxgi_resource->CreateSharedHandle(
          nullptr, DXGI_SHARED_RESOURCE_READ | DXGI_SHARED_RESOURCE_WRITE,
          nullptr, &texture_handle)))
    return handle;

  handle.dxgi_handle.set_handle(texture_handle);
  handle.type = gfx::DXGI_SHARED_HANDLE;
  handle.id = id;

  base::AutoLock lock(handles_lock_);
  std::pair<gfx::GpuMemoryBufferId, int> key = {id, client_id};
  handles_[key].Set(texture_handle);

  return handle;
}

void GpuMemoryBufferFactoryDXGI::DestroyGpuMemoryBuffer(
    gfx::GpuMemoryBufferId id,
    int client_id) {
  base::AutoLock lock(handles_lock_);
  std::pair<gfx::GpuMemoryBufferId, int> key = {id, client_id};
  handles_.erase(key);
}

ImageFactory* GpuMemoryBufferFactoryDXGI::AsImageFactory() {
  return this;
}

scoped_refptr<gl::GLImage>
GpuMemoryBufferFactoryDXGI::CreateImageForGpuMemoryBuffer(
    const gfx::GpuMemoryBufferHandle& handle,
    const gfx::Size& size,
    gfx::BufferFormat format,
    unsigned internalformat,
    int client_id,
    SurfaceHandle surface_handle) {
  // Transfer ownership of handle to return value.
  scoped_refptr<gl::GLImageDXGIHandle> ret(new gl::GLImageDXGIHandle(
      size, base::win::ScopedHandle(handle.dxgi_handle.get_handle()), 0));
  if (!ret->Initialize())
    ret = nullptr;
  return ret;
}

scoped_refptr<gl::GLImage> GpuMemoryBufferFactoryDXGI::CreateAnonymousImage(
    const gfx::Size& size,
    gfx::BufferFormat format,
    unsigned internalformat) {
  NOTIMPLEMENTED();
  scoped_refptr<gl::GLImageDXGI> ret;
  return ret;
}

unsigned GpuMemoryBufferFactoryDXGI::RequiredTextureType() {
  return GL_TEXTURE_2D;
}

bool GpuMemoryBufferFactoryDXGI::SupportsFormatRGB() {
  return true;
}
}  // namespace gpu