// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_GFX_ANDROID_HARDWARE_BUFFER_HANDLE_H_
#define UI_GFX_ANDROID_HARDWARE_BUFFER_HANDLE_H_

#include <stddef.h>
#include <stdint.h>

#include <vector>

#include "build/build_config.h"
#include "ui/gfx/gfx_export.h"

#if defined(OS_ANDROID)
#include "ui/gfx/android/scoped_android_hardware_buffer.h"
#endif

namespace gfx {

struct GFX_EXPORT AndroidHardwareBufferHandle {
  AndroidHardwareBufferHandle();
  AndroidHardwareBufferHandle(const AndroidHardwareBufferHandle& other);

  ~AndroidHardwareBufferHandle();

#if defined(OS_ANDROID)
  ScopedAndroidHardwareBuffer scoped_ahardwarebuffer;
#endif
};

#if defined(OS_ANDROID)
// Returns an instance of |handle| which can be sent over IPC. This increments
// the underlying AHardwareBuffer's refcount so that the IPC code take
// ownership of it, without invalidating |handle|.
AndroidHardwareBufferHandle CloneHandleForIPC(
    const AndroidHardwareBufferHandle& handle);
#endif

}  // namespace gfx

#endif  // UI_GFX_ANDROID_HARDWARE_BUFFER_HANDLE_H_
