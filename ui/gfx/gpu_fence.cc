// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/gfx/gpu_fence.h"

namespace gfx {

GpuFence::~GpuFence() = default;

GpuFence::GpuFence(const GpuFenceHandle& handle) : handle_(handle) {
  switch (handle.type) {
    case GpuFenceHandleType::EMPTY:
      break;
    case GpuFenceHandleType::ANDROID_NATIVE_FENCE_SYNC:
#if defined(OS_POSIX)
      if (handle.native_fd.auto_close)
        owned_fd_.reset(handle.native_fd.fd);
#endif
  }
}

GpuFenceHandle GpuFence::GetHandle() {
  return handle_;
}

ClientGpuFence GpuFence::AsClientGpuFence() {
  return reinterpret_cast<ClientGpuFence>(this);
}

// static
GpuFence* GpuFence::FromClientGpuFence(ClientGpuFence gpu_fence) {
  return reinterpret_cast<GpuFence*>(gpu_fence);
}

}  // namespace gfx
