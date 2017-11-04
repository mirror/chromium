// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/gfx/gpu_fence_handle.h"

#include "base/posix/eintr_wrapper.h"

#include <unistd.h>

namespace gfx {

GpuFenceHandle::GpuFenceHandle() : type(GpuFenceHandleType::EMPTY) {}

GpuFenceHandle::GpuFenceHandle(const GpuFenceHandle& other) = default;

GpuFenceHandle::~GpuFenceHandle() {
#if 0
  switch (type) {
    case GpuFenceHandleType::EMPTY:
      break;
    case GpuFenceHandleType::FILE_DESCRIPTOR:
      if (native_fd.fd >= 0)
        if (IGNORE_EINTR(close(native_fd.fd)) < 0)
          PLOG(ERROR) << "close";
      break;
  }
#endif
}

GpuFenceHandle CloneHandleForIPC(
    const GpuFenceHandle& source_handle) {
  switch (source_handle.type) {
    case GpuFenceHandleType::EMPTY:
      NOTREACHED();
      return source_handle;
    case GpuFenceHandleType::FILE_DESCRIPTOR: {
      gfx::GpuFenceHandle handle;
      handle.type = GpuFenceHandleType::FILE_DESCRIPTOR;
#if 0
      int duped_handle = HANDLE_EINTR(dup(source_handle.native_fd.fd));
#else
      // Don't dup them since currently nobody seems to close the original,
      // and closing in the destructor breaks copying. Sigh. Explicit Close()
      // via owning GpuFence?
      int duped_handle = source_handle.native_fd.fd;
#endif
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
