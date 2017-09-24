// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/gfx/android/scoped_android_hardware_buffer.h"

#include "base/logging.h"
#include "base/posix/unix_domain_socket.h"
#include "ui/gfx/geometry/size.h"

// We need to deal with three situations for AHardwareBuffer symbols:
//
// If the build uses a sufficiently new NDK: symbols always available.
// (This isn't the case currently, but we want to be ready for this.)
//
// Otherwise, when built using older NDKs, we need to use dynamic symbol
// loading. This loading will fail at runtime on pre-O systems, and is expected
// to succeed on Android O and newer.

#if __ANDROID_API__ >= 26

// We have support directly, no need to do anything beyond including the
// appropriate header.

#include "android/hardware_buffer.h"

namespace {

bool LoadAHardwareBufferFunctions() {
  return true;
}

void EnsureAHardwareBufferFunctionsLoaded() {
  // Nothing to do here.
}

}  // namespace

#else

// Need to use dynamic loading to look for the symbols at runtime, support
// depends on the target device. The definitions below are compatible
// with the NDK's android/hardware_buffer.h and must remain here until that's
// available for building directly.

#include <dlfcn.h>

namespace {

// Load the ABI compatibility definitions that would normally be in
// include/android/hardware_buffer.
#include "ui/gfx/android/android_hardware_buffer_abi.h"

// Function pointers as destinations for the dynamically loaded symbols.
PFAHardwareBuffer_allocate AHardwareBuffer_allocate;
PFAHardwareBuffer_acquire AHardwareBuffer_acquire;
PFAHardwareBuffer_describe AHardwareBuffer_describe;
PFAHardwareBuffer_sendHandleToUnixSocket AHardwareBuffer_sendHandleToUnixSocket;
PFAHardwareBuffer_recvHandleFromUnixSocket
    AHardwareBuffer_recvHandleFromUnixSocket;
PFAHardwareBuffer_release AHardwareBuffer_release;

// We want to try loading symbols one time only, since retries aren't going to
// help. However, this class lazy-loads them only when actually needed so that
// it can be used with null buffers on pre-Android-O devices without errors. So
// we need to separately track if we attempted loading, and if the load
// succeeded.
static bool DidCheckAHardwareBufferSupport = false;
static bool HaveAHardwareBufferSupport = false;

// Helper macro for early exit on load failures to reduce boilerplate.
#define LOAD_AHARDWAREBUFFER_FUNCTION(name)                      \
  *reinterpret_cast<void**>(&name) = dlsym(RTLD_DEFAULT, #name); \
  if (!name) {                                                   \
    LOG(ERROR) << "Unable to load " #name;                       \
    return false;                                                \
  }

bool LoadAHardwareBufferFunctions() {
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

void EnsureAHardwareBufferFunctionsLoaded() {
  bool have_support = LoadAHardwareBufferFunctions();
  DCHECK(have_support)
      << "Loading AHardwareBuffer symbols failed unexpectedly. "
      << "Did you forget to call IsAHardwareBufferSupportAvailable()?";
}

}  // namespace {}

#endif  // __ANDROID_API__ >= 26

namespace gfx {

ScopedAndroidHardwareBuffer::ScopedAndroidHardwareBuffer() {
  // LOG(INFO) << __FUNCTION__ << ";;; default constructor (EMPTY)";
}

ScopedAndroidHardwareBuffer::ScopedAndroidHardwareBuffer(
    RawAHardwareBuffer* buf) {
  // LOG(INFO) << __FUNCTION__ << ";;; FromBuffer constructor, buf=" << buf;
  reset(buf);
}

ScopedAndroidHardwareBuffer::ScopedAndroidHardwareBuffer(
    const ScopedAndroidHardwareBuffer& other) {
  // LOG(INFO) << __FUNCTION__ << ";;; Copy constructor";
  // CHECK(false) << "Why are you using copy constructor?";
  reset(other.get());
}

ScopedAndroidHardwareBuffer::ScopedAndroidHardwareBuffer(
    ScopedAndroidHardwareBuffer&& other) {
  // LOG(INFO) << __FUNCTION__ << ";;; Move constructor";
  // Don't use Acquire/Release (or reset which calls them) since refcounts
  // don't change on move.
  data_ = other.data_;
  other.data_ = nullptr;
}

ScopedAndroidHardwareBuffer::~ScopedAndroidHardwareBuffer() {
  // LOG(INFO) << __FUNCTION__ << ";;; destructor";
  reset();
}

ScopedAndroidHardwareBuffer& ScopedAndroidHardwareBuffer::operator=(
    const ScopedAndroidHardwareBuffer& other) {
  // LOG(INFO) << __FUNCTION__ << ";;; Copy assignment";
  if (this == &other)
    return *this;

  reset(other.get());

  return *this;
}

ScopedAndroidHardwareBuffer& ScopedAndroidHardwareBuffer::operator=(
    ScopedAndroidHardwareBuffer&& other) {
  // LOG(INFO) << __FUNCTION__ << ";;; Move assignment";
  if (this == &other)
    return *this;

  // Remove current content if any, and reduce refcount if needed.
  reset();

  // For the new content, don't use Acquire/Release (or reset which calls them)
  // since refcounts don't change on move.
  data_ = other.data_;
  other.data_ = nullptr;

  return *this;
}

// static
bool ScopedAndroidHardwareBuffer::IsAHardwareBufferSupportAvailable() {
  return LoadAHardwareBufferFunctions();
}

bool ScopedAndroidHardwareBuffer::is_valid() const {
  return data_ != nullptr;
}

void ScopedAndroidHardwareBuffer::reset(RawAHardwareBuffer* buf) {
  Release();
  data_ = buf;
  Acquire();
}

void ScopedAndroidHardwareBuffer::reset() {
  Release();
  data_ = nullptr;
}

RawAHardwareBuffer* ScopedAndroidHardwareBuffer::get() const {
  return data_;
}

void ScopedAndroidHardwareBuffer::Acquire() {
  if (!is_valid())
    return;
  // LOG(INFO) << __FUNCTION__ << ";;; buf=" << get();
  EnsureAHardwareBufferFunctionsLoaded();
  AHardwareBuffer_acquire(static_cast<AHardwareBuffer*>(data_));
}

void ScopedAndroidHardwareBuffer::Release() {
  if (!is_valid())
    return;
  // LOG(INFO) << __FUNCTION__ << ";;; buf=" << get();
  EnsureAHardwareBufferFunctionsLoaded();
  AHardwareBuffer_release(static_cast<AHardwareBuffer*>(data_));
}

void ScopedAndroidHardwareBuffer::AdoptFromSingleUseReadFD(
    base::ScopedFD read_fd) {
  // LOG(INFO) << __FUNCTION__ << ";;;";

  EnsureAHardwareBufferFunctionsLoaded();

  if (is_valid()) {
    reset();
  }

  // LOG(INFO) << __FUNCTION__ << ";;; read_fd=" << read_fd.get();
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

  // LOG(INFO) << __FUNCTION__ << ";;; ahb=" << ahb;

  // Do *not* call AHardwareBuffer_acquire(hbp), recvHandleFromUnixSocket did
  // so already. We do need to call release on it when we're done, i.e. after
  // binding it to an image. That happens in the destructor. We can't
  // preemptively release it here, that invalidates our accessor pointer.
  //
  // We don't need to use AHardwareBuffer_lock, that's just for accessing
  // memory from CPU which we're not currently doing.

  data_ = ahb;

#if 0
  AHardwareBuffer_Desc desc;
  AHardwareBuffer_describe(ahb, &desc);
  LOG(INFO) << __FUNCTION__ << ";;; DESC width=" << desc.width << " height=" << desc.height << std::hex << " format=0x" << desc.format << " usage=0x" << desc.usage;
#endif
}

base::ScopedFD ScopedAndroidHardwareBuffer::GetSingleUseReadFDForIPC() const {
  // LOG(INFO) << __FUNCTION__ << ";;;";

  // Can't get a transfer file descriptor for a null buffer. That may be ok
  // as long as the caller isn't intending to use it, so return an invalid
  // file descriptor.
  if (!is_valid()) {
    LOG(INFO) << ";;; no buffer, returning invalid FD";
    return base::ScopedFD();
  }

  EnsureAHardwareBufferFunctionsLoaded();

  base::ScopedFD read_fd, write_fd;
  base::CreateSocketPair(&read_fd, &write_fd);

  // LOG(INFO) << __FUNCTION__ << ";;; read_fd=" << read_fd.get();
  // LOG(INFO) << __FUNCTION__ << ";;; write_fd=" << write_fd.get();

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

// static
ScopedAndroidHardwareBuffer ScopedAndroidHardwareBuffer::CreateFromSpec(
    const Size& size,
    BufferFormat format,
    BufferUsage usage) {
  // LOG(INFO) << __FUNCTION__ << ";;;";

  EnsureAHardwareBufferFunctionsLoaded();

  AHardwareBuffer_Desc desc;
  // On create, all elements must be initialized, including setting the
  // "reserved for future use" (rfu) fields to zero.
  desc.width = size.width();
  desc.height = size.height();
  desc.layers = 1;  // number of images
  desc.stride = 0;  // ignored for AHardwareBuffer_allocate
  desc.rfu0 = 0;
  desc.rfu1 = 0;

  switch (format) {
    case BufferFormat::RGBA_8888:
      desc.format = AHARDWAREBUFFER_FORMAT_R8G8B8A8_UNORM;
      break;
    case BufferFormat::RGBX_8888:
      desc.format = AHARDWAREBUFFER_FORMAT_R8G8B8_UNORM;
      break;
    case BufferFormat::BGR_565:
      desc.format = AHARDWAREBUFFER_FORMAT_R5G6B5_UNORM;
      break;
    default:
      LOG(ERROR) << __FUNCTION__ << ": Unsupported gfx::BufferFormat: "
                 << static_cast<int>(format);
      return ScopedAndroidHardwareBuffer();
  }

  switch (usage) {
    case BufferUsage::SCANOUT:
      desc.usage = AHARDWAREBUFFER_USAGE_GPU_SAMPLED_IMAGE |
                   AHARDWAREBUFFER_USAGE_GPU_COLOR_OUTPUT;
      break;
    default:
      LOG(ERROR) << __FUNCTION__ << ": Unsupported gfx::BufferUsage: "
                 << static_cast<int>(usage);
      return ScopedAndroidHardwareBuffer();
  }

  AHardwareBuffer* ahb = nullptr;
  AHardwareBuffer_allocate(&desc, &ahb);
  if (!ahb) {
    LOG(ERROR) << __FUNCTION__ << ": Failed to allocate AHardwareBuffer";
    return ScopedAndroidHardwareBuffer();
  }

#if 0
  AHardwareBuffer_describe(ahb, &desc);
  LOG(INFO) << __FUNCTION__ << ";;; DESC width=" << desc.width << " height=" << desc.height << std::hex << " format=0x" << desc.format << " usage=0x" << desc.usage;
#endif

  return ScopedAndroidHardwareBuffer(static_cast<RawAHardwareBuffer*>(ahb));
}

}  // namespace gfx
