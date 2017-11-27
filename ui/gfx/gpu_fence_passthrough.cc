// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/gfx/gpu_fence_passthrough.h"

#include "base/logging.h"

namespace gfx {

GpuFencePassthrough::GpuFencePassthrough(const GpuFenceHandle& handle)
    : type_(handle.type) {
  switch (type_) {
    case GpuFenceHandleType::EMPTY:
      break;
    case GpuFenceHandleType::ANDROID_NATIVE_FENCE_SYNC:
#if defined(OS_POSIX)
      owned_fd_.reset(handle.native_fd.fd);
#else
      NOTREACHED();
#endif
      break;
  }
}

GpuFencePassthrough::~GpuFencePassthrough() = default;

GpuFenceHandle GpuFencePassthrough::GetGpuFenceHandle() {
  gfx::GpuFenceHandle handle;
  switch (type_) {
    case GpuFenceHandleType::EMPTY:
      break;
    case GpuFenceHandleType::ANDROID_NATIVE_FENCE_SYNC:
#if defined(OS_POSIX)
      handle.type = gfx::GpuFenceHandleType::ANDROID_NATIVE_FENCE_SYNC;
      handle.native_fd = base::FileDescriptor(owned_fd_.get(),
                                              /*auto_close=*/false);
#else
      NOTREACHED();
#endif
      break;
  }
  return handle;
}

}  // namespace gfx
