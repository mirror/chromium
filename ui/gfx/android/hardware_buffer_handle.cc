// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/gfx/android/hardware_buffer_handle.h"

#if defined(OS_ANDROID)
#include "ui/gfx/android/scoped_android_hardware_buffer.h"
#endif

namespace gfx {

AndroidHardwareBufferHandle::AndroidHardwareBufferHandle() {}
AndroidHardwareBufferHandle::AndroidHardwareBufferHandle(
    const AndroidHardwareBufferHandle& other) = default;

AndroidHardwareBufferHandle::~AndroidHardwareBufferHandle() {}

AndroidHardwareBufferHandle CloneHandleForIPC(
    const AndroidHardwareBufferHandle& handle) {
  AndroidHardwareBufferHandle clone;
#if defined(OS_ANDROID)
  if (handle.scoped_ahardwarebuffer.is_valid()) {
    clone.scoped_ahardwarebuffer.reset(handle.scoped_ahardwarebuffer.get());
  }
#endif  // defined(OS_ANDROID)
  return clone;
}

}  // namespace gfx
