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

namespace base {

// This class provides runtime support for working with AHardwareBuffer objects
// on Android O systems without requiring building for the Android O NDK level.
class BASE_EXPORT AndroidHardwareBufferCompat {
 public:
  /// Checks if runtime support is available by trying to load the functions
  /// dynamically. If it succeeds, the functions have been loaded.
  static bool IsSupportAvailable();
};

}  // namespace base

#endif  // BASE_ANDROID_ANDROID_HARDWARE_BUFFER_COMPAT_H_
