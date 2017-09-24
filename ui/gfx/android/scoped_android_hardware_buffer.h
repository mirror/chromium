// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_GFX_ANDROID_SCOPED_ANDROID_HARDWARE_BUFFER_H_
#define UI_GFX_ANDROID_SCOPED_ANDROID_HARDWARE_BUFFER_H_

#include "base/files/scoped_file.h"
#include "base/scoped_generic.h"
#include "ui/gfx/buffer_types.h"
#include "ui/gfx/gfx_export.h"

namespace gfx {

class Size;

// Make RawAHardwareBuffer* usable as an alias to AHardwareBuffer*. Using the
// real definition is tricky. The official definition only exists when building
// for Android API level >= 26 (which Chrome currently doesn't do), and SkImage
// provides its own definition which can cause conflicts.
typedef void RawAHardwareBuffer;

// Wrapper around an AHardwareBuffer* object with support for managing
// reference counts and IPC transfer.
//
// This class can be used on any Android system. On pre-O versions that do not
// have AHardwareBuffer support, the wrapped pointer is always nullptr, and
// is_valid() returns false.
//
// You can use the static IsAHardwareBufferSupportAvailable() method to check
// if AHardwareBuffer support is available. This is not a requirement, but if
// you assign a non-null AHardwareBuffer* (via constructor, reset(), or
// AdoptFromSingleUseReadFD()), the code will assume that support is available,
// and will fail at runtime if dynamic loading of the support functions fails.
//
// The class manages underlying AHardwareBuffer's ref count via acquire() and
// release() automatically, and supports copy and move semantics.
class GFX_EXPORT ScopedAndroidHardwareBuffer {
 public:
  ScopedAndroidHardwareBuffer();
  ScopedAndroidHardwareBuffer(RawAHardwareBuffer* buf);
  ~ScopedAndroidHardwareBuffer();

  // Copy and assignment operators including move semantics
  ScopedAndroidHardwareBuffer(const ScopedAndroidHardwareBuffer& other);
  ScopedAndroidHardwareBuffer(ScopedAndroidHardwareBuffer&& other);
  ScopedAndroidHardwareBuffer& operator=(
      const ScopedAndroidHardwareBuffer& other);
  ScopedAndroidHardwareBuffer& operator=(ScopedAndroidHardwareBuffer&& other);

  static bool IsAHardwareBufferSupportAvailable();

  bool is_valid() const;
  void reset();
  void reset(RawAHardwareBuffer* buf);
  RawAHardwareBuffer* get() const;

  // Create an IPC-transferable file descriptor. Does not modify ref count,
  // this method is usually called after CloneHandleForIPC which already did
  // that. If it's read successfully at the other end, that constructor will
  // increment refcount for further use.
  base::ScopedFD GetSingleUseReadFDForIPC() const;

  // Consume an IPC-transferable file descriptor. The ref count is incremented
  // when read successfully.
  void AdoptFromSingleUseReadFD(base::ScopedFD single_use_read_fd_for_ipc);

  static ScopedAndroidHardwareBuffer CreateFromSpec(const Size&,
                                                    BufferFormat,
                                                    BufferUsage);

 private:
  RawAHardwareBuffer* data_ = nullptr;
  void Acquire();
  void Release();
};

}  // namespace gfx

#endif  // UI_GFX_ANDROID_SCOPED_ANDROID_HARDWARE_BUFFER_H_
