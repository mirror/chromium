// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/android/android_hardware_buffer_compat.h"

#include "base/logging.h"
#include "base/posix/unix_domain_socket.h"

#undef DVLOG
#define DVLOG(X) LOG(ERROR)

namespace base {

// We need to deal with three situations for AHardwareBuffer symbols:
//
// If the build uses a sufficiently new NDK: symbols always available.
// (This isn't the case currently, but we want to be ready for this.)
//
// Otherwise, when built using older NDKs, we need to use dynamic symbol
// loading. This loading will fail at runtime on pre-O systems, and is expected
// to succeed on Android O and newer.

#if __ANDROID_API__ >= 26

// We have support directly, no need for any dynamic loading.

bool AndroidHardwareBufferCompat::LoadFunctions() {
  return true;
}

void AndroidHardwareBufferCompat::EnsureFunctionsLoaded() {
  // Nothing to do here.
}

#else

// Need to use dynamic loading to look for the symbols at runtime, support
// depends on the target device.

#include <dlfcn.h>

namespace {

// We want to try loading symbols one time only, since retries aren't going to
// help. However, this class lazy-loads them only when actually needed so that
// it can be used with null buffers on pre-Android-O devices without errors. So
// we need to separately track if we attempted loading, and if the load
// succeeded.
static bool DidCheckAHardwareBufferSupport = false;
static bool HaveAHardwareBufferSupport = false;

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
#define LOAD_AHARDWAREBUFFER_FUNCTION(name)                      \
  *reinterpret_cast<void**>(&name) = dlsym(RTLD_DEFAULT, #name); \
  if (!name) {                                                   \
    LOG(ERROR) << "Unable to load " #name;                       \
    return false;                                                \
  }

bool LoadFunctions() {
  // Do this runtime check once only, the result won't change.
  if (DidCheckAHardwareBufferSupport)
    return HaveAHardwareBufferSupport;

  DidCheckAHardwareBufferSupport = true;

  // The following macro expansions will return early if symbol loading fails,
  // and HaveAHardwareBufferSupport stays false in that case.
  LOAD_AHARDWAREBUFFER_FUNCTION(AHardwareBuffer_allocate);
  LOAD_AHARDWAREBUFFER_FUNCTION(AHardwareBuffer_acquire);
  LOAD_AHARDWAREBUFFER_FUNCTION(AHardwareBuffer_describe);
  LOAD_AHARDWAREBUFFER_FUNCTION(AHardwareBuffer_sendHandleToUnixSocket);
  LOAD_AHARDWAREBUFFER_FUNCTION(AHardwareBuffer_recvHandleFromUnixSocket);
  LOAD_AHARDWAREBUFFER_FUNCTION(AHardwareBuffer_release);

  // If we get here, all the symbols we asked for were loaded successfully.
  HaveAHardwareBufferSupport = true;
  return true;
}

}  // namespace

void AndroidHardwareBufferCompat::EnsureFunctionsLoaded() {
  bool have_support = LoadFunctions();
  DCHECK(have_support)
      << "Loading AHardwareBuffer symbols failed unexpectedly. "
      << "Did you forget to call IsSupportAvailable()?";
}

#endif  // __ANDROID_API__ >= 26

// static
bool AndroidHardwareBufferCompat::IsSupportAvailable() {
  return LoadFunctions();
}

}  // namespace base
