// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "gpu/ipc/common/gpu_fence_impl.h"

#if defined(USE_EGL)
#include "gpu/ipc/common/gpu_fence_impl_android_native_fence_sync.h"
#include "ui/gl/gl_surface_egl.h"
#endif

namespace gpu {

GpuFenceImpl::GpuFenceImpl() {}

// static
bool GpuFenceImpl::IsSupported() {
#if defined USE_EGL
  return gl::GLSurfaceEGL::IsAndroidNativeFenceSyncSupported();
#endif
  return false;
}

// static
std::unique_ptr<GpuFenceImpl> GpuFenceImpl::CreateFromHandle(
    const gfx::GpuFenceHandle& handle) {
  DCHECK(IsSupported());
  switch (handle.type) {
#if defined(USE_EGL)
    case gfx::GpuFenceHandleType::ANDROID_NATIVE_FENCE_SYNC:
      return base::MakeUnique<GpuFenceImplAndroidNativeFenceSync>(handle);
#endif
    default:
      NOTREACHED();
      return nullptr;
  }
}

// static
std::unique_ptr<GpuFenceImpl> GpuFenceImpl::CreateNew() {
  DCHECK(IsSupported());
#if defined USE_EGL
  return base::MakeUnique<GpuFenceImplAndroidNativeFenceSync>();
#endif
  NOTREACHED();
  return nullptr;
}

}  // namespace gpu
