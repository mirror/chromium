// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/gfx/gpu_fence.h"

#include "base/logging.h"
#include "base/posix/eintr_wrapper.h"

namespace gfx {

GpuFence::~GpuFence() {
  // LOG(INFO) << __FUNCTION__ << ";;; fence_=" << fence_.get();
  switch (handle_.type) {
    case GpuFenceHandleType::EMPTY:
      break;
    case GpuFenceHandleType::ANDROID_NATIVE_FENCE_SYNC:
      if (handle_.native_fd.fd >= 0)
        if (IGNORE_EINTR(close(handle_.native_fd.fd)) < 0)
          PLOG(ERROR) << "close";
      break;
  }
}

GpuFence::GpuFence(const GpuFenceHandle& handle) : handle_(handle) {}

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
