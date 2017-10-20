// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/gfx/android/android_hardware_buffer_compat.h"

#include "base/logging.h"
#include "base/posix/unix_domain_socket.h"
#include "ui/gfx/geometry/size.h"

#undef DVLOG
#define DVLOG(X) LOG(ERROR)

namespace  gfx {

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

}  // namespace {}

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

#if 0
void ScopedAndroidHardwareBuffer::Acquire() {
  if (!is_valid())
    return;
  DVLOG(2) << __FUNCTION__ << ": Acquire buf=" << get();
  EnsureFunctionsLoaded();
  AHardwareBuffer_acquire(static_cast<AHardwareBuffer*>(data_));
}

void ScopedAndroidHardwareBuffer::Release() {
  if (!is_valid())
    return;
  DVLOG(2) << __FUNCTION__ << ": Release buf=" << get();
  EnsureFunctionsLoaded();
  AHardwareBuffer_release(static_cast<AHardwareBuffer*>(data_));
}

void ScopedAndroidHardwareBuffer::AdoptFromSingleUseReadFD(
    base::ScopedFD read_fd) {
  DVLOG(2) << __FUNCTION__ << ": read_fd=" << read_fd.get();

  EnsureFunctionsLoaded();

  if (is_valid()) {
    reset();
  }

  if (!read_fd.is_valid()) {
    LOG(ERROR) << __FUNCTION__ << ";;; Invalid file descriptor";
    return;
  }

  AHardwareBuffer* ahb = nullptr;

  int ret = AHardwareBuffer_recvHandleFromUnixSocket(read_fd.get(), &ahb);

  if (ret < 0) {
    LOG(ERROR) << __FUNCTION__ << ";;; recv failed! FIXME!";
    return;
  }

  // Do *not* call AHardwareBuffer_acquire(hbp), recvHandleFromUnixSocket did
  // so already. We do need to call release on it when we're done, i.e. after
  // binding it to an image. That happens in the destructor. We can't
  // preemptively release it here, that invalidates our accessor pointer.
  //
  // We don't need to use AHardwareBuffer_lock, that's just for accessing
  // memory from CPU which we're not currently doing.

  data_ = ahb;
}

base::ScopedFD ScopedAndroidHardwareBuffer::GetSingleUseReadFDForIPC() const {
  DVLOG(2) << __FUNCTION__;

  // Can't get a transfer file descriptor for a null buffer. That may be ok
  // as long as the caller isn't intending to use it, so return an invalid
  // file descriptor.
  if (!is_valid()) {
    DVLOG(2) << __FUNCTION__ << ": no valid buffer, returning invalid FD";
    return base::ScopedFD();
  }

  EnsureFunctionsLoaded();

  base::ScopedFD read_fd, write_fd;
  base::CreateSocketPair(&read_fd, &write_fd);

  int ret = AHardwareBuffer_sendHandleToUnixSocket(
      static_cast<AHardwareBuffer*>(get()), write_fd.get());

  if (ret < 0) {
    LOG(ERROR) << __FUNCTION__ << ";;; send failed! FIXME!";
    return base::ScopedFD();
  }

  // Close now to avoid timeouts in case receiver goes away? cf. elsewhere.
  write_fd.reset();

  return read_fd;
}

#endif

// static
AHardwareBuffer* AndroidHardwareBufferCompat::CreateFromSpec(
    const Size& size,
    BufferFormat format,
    BufferUsage usage) {
  DVLOG(2) << __FUNCTION__;

  EnsureFunctionsLoaded();

  AHardwareBuffer_Desc desc;
  // On create, all elements must be initialized, including setting the
  // "reserved for future use" (rfu) fields to zero.
  desc.width = size.width();
  desc.height = size.height();
  desc.layers = 1;  // number of images
  desc.stride = 0;  // ignored for AHardwareBuffer_allocate
  desc.rfu0 = 0;
  desc.rfu1 = 0;

  // TODO(klausw): support BGR_565? Native type's color order doesn't match:
  // AHARDWAREBUFFER_FORMAT_R5G6B5_UNORM
  switch (format) {
    case BufferFormat::RGBA_8888:
      desc.format = AHARDWAREBUFFER_FORMAT_R8G8B8A8_UNORM;
      break;
    case BufferFormat::RGBX_8888:
      desc.format = AHARDWAREBUFFER_FORMAT_R8G8B8_UNORM;
      break;
    default:
      LOG(ERROR) << __FUNCTION__ << ": Unsupported gfx::BufferFormat: "
                 << static_cast<int>(format);
      return nullptr;
  }

  switch (usage) {
    case BufferUsage::SCANOUT:
      desc.usage = AHARDWAREBUFFER_USAGE_GPU_SAMPLED_IMAGE |
                   AHARDWAREBUFFER_USAGE_GPU_COLOR_OUTPUT;
      break;
    default:
      LOG(ERROR) << __FUNCTION__ << ": Unsupported gfx::BufferUsage: "
                 << static_cast<int>(usage);
      return nullptr;
  }

  AHardwareBuffer* ahb = nullptr;
  AHardwareBuffer_allocate(&desc, &ahb);
  if (!ahb) {
    LOG(ERROR) << __FUNCTION__ << ": Failed to allocate AHardwareBuffer";
    return nullptr;
  }

  return ahb;
}

}  // namespace gfx
