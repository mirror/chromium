// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef BASE_ANDROID_ANDROID_HARDWARE_BUFFER_COMPAT_H_
#define BASE_ANDROID_ANDROID_HARDWARE_BUFFER_COMPAT_H_

#include "base/base_export.h"

#if __ANDROID_API__ >= 26
#include <android/hardware_buffer.h>
#else
#include "base/android/android_hardware_buffer_abi.h"
#endif

// The following typedef is technically redundant, but add it here anyway
// as used by forward declarations elsewhere to ensure that it's compatible.
extern "C" typedef struct AHardwareBuffer AHardwareBuffer;

namespace base {

// This class provides runtime support for working with AHardwareBuffer objects
// on Android O systems without requiring building for the Android O NDK level.
//
// You MUST use the static IsAHardwareBufferSupportAvailable() method to check
// if AHardwareBuffer support is available, and call EnsureFunctionsLoaded() at
// least once per process for any method that assumes that they are present.
class BASE_EXPORT AndroidHardwareBufferCompat {
 public:
  static bool IsSupportAvailable();
  static void EnsureFunctionsLoaded();
};

}  // namespace base

#endif  // BASE_ANDROID_ANDROID_HARDWARE_BUFFER_COMPAT_H_
