// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/gfx/gpu_fence_handle.h"

#include "base/posix/eintr_wrapper.h"

#include <unistd.h>

namespace gfx {

GpuFenceHandle::GpuFenceHandle() : type(GpuFenceHandleType::EMPTY) {}

GpuFenceHandle::GpuFenceHandle(const GpuFenceHandle& other) = default;

GpuFenceHandle::~GpuFenceHandle() {}

GpuFenceHandle CloneHandleForIPC(
    const GpuFenceHandle& source_handle) {
  switch (source_handle.type) {
    case GpuFenceHandleType::EMPTY:
      NOTREACHED();
      return source_handle;
    case GpuFenceHandleType::FILE_DESCRIPTOR: {
      gfx::GpuFenceHandle handle;
      handle.type = GpuFenceHandleType::FILE_DESCRIPTOR;
      int duped_handle = HANDLE_EINTR(dup(source_handle.native_fd.fd));
      if (duped_handle < 0)
        return GpuFenceHandle();
      handle.native_fd = base::FileDescriptor(duped_handle, true);
      return handle;
    }
  }
  NOTREACHED();
  return gfx::GpuFenceHandle();
}

}  // namespace gfx
