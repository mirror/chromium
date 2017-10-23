// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/android/android_hardware_buffer_compat.h"

#include "base/android/build_info.h"
#include "base/lazy_instance.h"
#include "base/logging.h"
#include "base/synchronization/lock.h"
#include "build/build_config.h"

namespace base {

// We need to deal with three situations for AHardwareBuffer symbols:
//
// If the build requires a sufficiently new NDK: symbols always available.
// (This isn't the case currently, but we want to be ready for this.)
//
// Otherwise, when built using older NDKs, we need to use dynamic symbol
// loading. This loading will fail at runtime on pre-O systems, and is expected
// to succeed on Android O and newer.

#if __ANDROID_API__ >= 26

// We have support directly, no need for any dynamic loading.

bool AndroidHardwareBufferCompat::IsSupportAvailable() {
  return true;
}

#else

// Need to use dynamic loading to look for the symbols at runtime, support
// depends on the target device.

#include <dlfcn.h>

namespace {

// We want to try loading symbols one time only, since retries aren't going to
// help. We start out as "unknown", and the status is set to available or not
// based on the result of the first loading attempt.
enum class LoadingStatus { NOT_CHECKED_YET, SUCCESS, FAILURE };
static base::LazyInstance<base::Lock>::Leaky g_loading_lock =
    LAZY_INSTANCE_INITIALIZER;
static LoadingStatus g_ahardwarebuffer_support = LoadingStatus::NOT_CHECKED_YET;

extern "C" {
PFAHardwareBuffer_allocate AHardwareBuffer_allocate;
PFAHardwareBuffer_acquire AHardwareBuffer_acquire;
PFAHardwareBuffer_describe AHardwareBuffer_describe;
PFAHardwareBuffer_sendHandleToUnixSocket AHardwareBuffer_sendHandleToUnixSocket;
PFAHardwareBuffer_recvHandleFromUnixSocket
    AHardwareBuffer_recvHandleFromUnixSocket;
PFAHardwareBuffer_release AHardwareBuffer_release;
}

// Helper macro for early exit on load failures to reduce boilerplate.
#define LOAD_AHARDWAREBUFFER_FUNCTION(name)                        \
  *reinterpret_cast<void**>(&name) = dlsym(main_dl_handle, #name); \
  if (!name) {                                                     \
    DVLOG(1) << "Unable to load " #name;                           \
    return false;                                                  \
  }

bool LoadFunctions() {
  // If the runtime version is too old, assume not supported. Android O and up
  // should have support, but be prepared for dynamic loading to fail to avoid
  // crashes on devices that don't fully follow the CDD requirements.
  if (base::android::BuildInfo::GetInstance()->sdk_int() <
      base::android::SDK_VERSION_OREO)
    return false;

  // cf. base/android/linker/modern_linker_jni.cc
  static void* main_dl_handle = nullptr;
  if (!main_dl_handle)
    main_dl_handle = dlopen(nullptr, RTLD_NOW);

  // The following macro expansions will return early if symbol loading fails,
  // and g_have_ahardwarebuffer_support stays false in that case.
  LOAD_AHARDWAREBUFFER_FUNCTION(AHardwareBuffer_allocate);
  LOAD_AHARDWAREBUFFER_FUNCTION(AHardwareBuffer_acquire);
  LOAD_AHARDWAREBUFFER_FUNCTION(AHardwareBuffer_describe);
  LOAD_AHARDWAREBUFFER_FUNCTION(AHardwareBuffer_sendHandleToUnixSocket);
  LOAD_AHARDWAREBUFFER_FUNCTION(AHardwareBuffer_recvHandleFromUnixSocket);
  LOAD_AHARDWAREBUFFER_FUNCTION(AHardwareBuffer_release);

  // If we get here, all the symbols we asked for were loaded successfully.
  return true;
}

}  // namespace

// static
bool AndroidHardwareBufferCompat::IsSupportAvailable() {
  // Do this runtime check once only, the result won't change.
  // If we've already checked, return the previous result.
  if (g_ahardwarebuffer_support != LoadingStatus::NOT_CHECKED_YET)
    return g_ahardwarebuffer_support == LoadingStatus::SUCCESS;

  // Protect global state changes from race conditions, and modify
  // the variable away from NOT_CHECKED_YET one time only.
  base::AutoLock auto_lock(g_loading_lock.Get());
  if (LoadFunctions()) {
    g_ahardwarebuffer_support = LoadingStatus::SUCCESS;
    return true;
  } else {
    g_ahardwarebuffer_support = LoadingStatus::FAILURE;
    return false;
  }
}

#endif  // __ANDROID_API__ >= 26

}  // namespace base
