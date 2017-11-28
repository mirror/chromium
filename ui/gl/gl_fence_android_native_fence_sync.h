// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_GL_GL_FENCE_ANDROID_NATIVE_FENCE_SYNC_H_
#define UI_GL_GL_FENCE_ANDROID_NATIVE_FENCE_SYNC_H_

#include "base/files/scoped_file.h"
#include "base/macros.h"
#include "build/build_config.h"
#include "ui/gl/gl_export.h"
#include "ui/gl/gl_fence_egl.h"

namespace gl {

class GL_EXPORT GLFenceAndroidNativeFenceSync : public GLFenceEGL {
 public:
  ~GLFenceAndroidNativeFenceSync() override;

  static std::unique_ptr<GLFenceAndroidNativeFenceSync> CreateGpuFence();

  static std::unique_ptr<GLFenceAndroidNativeFenceSync>
  CreateFromGpuFenceHandle(const gfx::GpuFenceHandle&);

  gfx::GpuFenceHandle GetGpuFenceHandle() override;

 private:
  GLFenceAndroidNativeFenceSync();
  static std::unique_ptr<GLFenceAndroidNativeFenceSync> CreateInternal(
      EGLenum type,
      EGLint* attribs);

 private:
#if defined(OS_POSIX)
  // Technically, we could extract the underlying FD as needed from the
  // underlying native sync object, but this would require making EGL calls as
  // part of IPC transport. To avoid this, create the file descriptor as part
  // of creating the object to ensure we use the client's current EGL context.
  base::ScopedFD owned_fd_;
#endif
};

}  // namespace gl

#endif  // UI_GL_GL_FENCE_ANDROID_NATIVE_FENCE_SYNC_H_
