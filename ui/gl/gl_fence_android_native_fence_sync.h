// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_GL_GL_FENCE_ANDROID_NATIVE_FENCE_SYNC_H_
#define UI_GL_GL_FENCE_ANDROID_NATIVE_FENCE_SYNC_H_

#include "base/macros.h"
#include "ui/gl/gl_export.h"
#include "ui/gl/gl_fence_egl.h"

namespace gl {

class GL_EXPORT GLFenceAndroidNativeFenceSync : public GLFenceEGL {
 public:
  GLFenceAndroidNativeFenceSync();
  static std::unique_ptr<GLFenceEGL> CreateFromGpuFenceHandle(
      const gfx::GpuFenceHandle&);

  gfx::GpuFenceHandle GetGpuFenceHandle() override;
};

}  // namespace gl

#endif  // UI_GL_GL_FENCE_ANDROID_NATIVE_FENCE_SYNC_H_
