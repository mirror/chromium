// Copyright (c) 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_GL_GL_IMAGE_AHARDWAREBUFFER_H_
#define UI_GL_GL_IMAGE_AHARDWAREBUFFER_H_

#include "base/macros.h"
#include "base/threading/thread_checker.h"
#include "ui/gl/gl_bindings.h"
#include "ui/gl/gl_export.h"
#include "ui/gl/gl_image_egl.h"

namespace gl {

class GL_EXPORT GLImageAHardwareBuffer : public GLImageEGL {
 public:
  explicit GLImageAHardwareBuffer(const gfx::Size& size);

  bool Initialize(EGLenum target, EGLClientBuffer buffer, const EGLint* attrs);

  // Overridden from GLImage:
  void OnMemoryDump(base::trace_event::ProcessMemoryDump* pmd,
                    uint64_t process_tracing_id,
                    const std::string& dump_name) override;

 protected:
  ~GLImageAHardwareBuffer() override;

 private:
  DISALLOW_COPY_AND_ASSIGN(GLImageAHardwareBuffer);
};

}  // namespace gl

#endif  // UI_GL_GL_IMAGE_AHARDWAREBUFFER_H_
