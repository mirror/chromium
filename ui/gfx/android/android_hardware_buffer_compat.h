// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_GFX_ANDROID_ANDROID_HARDWARE_BUFFER_COMPAT_H_
#define UI_GFX_ANDROID_ANDROID_HARDWARE_BUFFER_COMPAT_H_

#include "ui/gfx/buffer_types.h"
#include "ui/gfx/gfx_export.h"

#if __ANDROID_API__ >= 26
#include <android/hardware_buffer.h>
#else
#include "ui/gfx/android/android_hardware_buffer_abi.h"
#endif

// The following typedef is technically redundant, but add it here anyway
// as used by forward declarations elsewhere to ensure that it's compatible.
extern "C" typedef struct AHardwareBuffer AHardwareBuffer;

namespace gfx {

class Size;

// This class provides runtime support for working with AHardwareBuffer objects
// on Android O systems without requiring building for the Android O NDK level.
//
// You MUST use the static IsAHardwareBufferSupportAvailable() method to check
// if AHardwareBuffer support is available, and call EnsureFunctionsLoaded() at
// least once per process for any method that assumes that they are present.
class GFX_EXPORT AndroidHardwareBufferCompat {
 public:
  static bool IsSupportAvailable();
  static void EnsureFunctionsLoaded();
  static AHardwareBuffer* CreateFromSpec(const Size&,
                                         BufferFormat,
                                         BufferUsage);
};

}  // namespace gfx

#endif  // UI_GFX_ANDROID_ANDROID_HARDWARE_BUFFER_COMPAT_H_
