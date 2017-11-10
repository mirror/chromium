// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/gfx/gpu_fence_handle.h"

#if defined(OS_POSIX)
#include <unistd.h>

#include "base/posix/eintr_wrapper.h"
#endif

namespace gfx {

GpuFenceHandle::GpuFenceHandle() : type(GpuFenceHandleType::EMPTY) {}

GpuFenceHandle::GpuFenceHandle(const GpuFenceHandle& other) = default;

GpuFenceHandle::~GpuFenceHandle() {}

void GpuFenceHandle::Close() {
  switch (type) {
    case GpuFenceHandleType::EMPTY:
      break;
    case GpuFenceHandleType::ANDROID_NATIVE_FENCE_SYNC:
#if defined(OS_POSIX)
      if (native_fd.auto_close && native_fd.fd >= 0)
        if (IGNORE_EINTR(close(native_fd.fd)) < 0)
          PLOG(ERROR) << "close";
#endif
      break;
  }
}

GpuFenceHandle CloneHandleForIPC(const GpuFenceHandle& source_handle) {
  switch (source_handle.type) {
    case GpuFenceHandleType::EMPTY:
      NOTREACHED();
      return source_handle;
    case GpuFenceHandleType::ANDROID_NATIVE_FENCE_SYNC: {
      gfx::GpuFenceHandle handle;
#if defined(OS_POSIX)
      handle.type = GpuFenceHandleType::ANDROID_NATIVE_FENCE_SYNC;
      int duped_handle = HANDLE_EINTR(dup(source_handle.native_fd.fd));
      if (duped_handle < 0)
        return GpuFenceHandle();
      handle.native_fd = base::FileDescriptor(duped_handle, true);
#endif
      return handle;
    }
  }
  NOTREACHED();
  return gfx::GpuFenceHandle();
}

}  // namespace gfx
