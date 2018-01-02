// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/vr/test/gl_test_environment.h"
#include "ui/gl/gl_version_info.h"
#include "ui/gl/init/gl_factory.h"
#include "ui/gl/test/gl_image_test_support.h"
#include "ui/gl/test/gl_test_helper.h"
#include "ui/gl/test/gl_image_test_template.h"
#include "ui/gl/test/gl_surface_test_support.h"
#include "ui/gl/gl_surface_egl.h"
#include "ui/gl/gl_image_native_pixmap.h"

namespace vr {

GlTestEnvironment::GlTestEnvironment(const gfx::Size frame_buffer_size) {
  // Setup offscreen GL context.
  gl::GLImageTestSupport::InitializeGL();
  // std::vector<gl::GLImplementation> allowed_impls =
  //     gl::init::GetAllowedGLImplementations();
  // CHECK(!allowed_impls.empty());
  // CHECK(std::find(allowed_impls.begin(), allowed_impls.end(), gl::kGLImplementationEGLGLES2) != allowed_impls.end());

  // // GLImplementation impl = allowed_impls[0];
  // gl::GLSurfaceTestSupport::InitializeOneOffImplementation(gl::kGLImplementationEGLGLES2, true);

  surface_ = gl::init::CreateOffscreenGLSurface(gfx::Size());

  context_ = gl::init::CreateGLContext(nullptr, surface_.get(),
                                       gl::GLContextAttribs());
  context_->MakeCurrent(surface_.get());


  // gl::GLSurfaceEGL::InitializeDisplay(static_cast<EGLNativeDisplayType>(surface_->GetDisplay()));
  // scoped_refptr<gl::GLImageNativePixmap> image(
  //     new gl::GLImageNativePixmap(gfx::Size(500, 500), GL_RGBA));
  // CHECK(image->Initialize(pixmap.get(), gfx::BufferFormat::RGBA_8888));

  if (gl::GLContext::GetCurrent()->GetVersionInfo()->IsAtLeastGL(3, 3)) {
    // To avoid glGetVertexAttribiv(0, ...) failing.
    glGenVertexArraysOES(1, &vao_);
    glBindVertexArrayOES(vao_);
  }

  frame_buffer_ = gl::GLTestHelper::SetupFramebuffer(
      frame_buffer_size.width(), frame_buffer_size.height());
  glBindFramebufferEXT(GL_FRAMEBUFFER, frame_buffer_);

  // Create depth buffer.
  glGenRenderbuffersEXT(1, &depth_render_buffer_);
  glBindRenderbufferEXT(GL_RENDERBUFFER, depth_render_buffer_);
  glRenderbufferStorageEXT(GL_RENDERBUFFER, GL_DEPTH_COMPONENT16,
                           frame_buffer_size.width(),
                           frame_buffer_size.height());
  glFramebufferRenderbufferEXT(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,
                               GL_RENDERBUFFER, depth_render_buffer_);
  glBindRenderbufferEXT(GL_RENDERBUFFER, 0);
}

GlTestEnvironment::~GlTestEnvironment() {
  glDeleteFramebuffersEXT(1, &frame_buffer_);
  if (vao_) {
    glDeleteVertexArraysOES(1, &vao_);
  }
  context_->ReleaseCurrent(surface_.get());
  context_ = nullptr;
  surface_ = nullptr;
  gl::GLImageTestSupport::CleanupGL();
}

GLuint GlTestEnvironment::MakeTextureExternalOes(const gfx::Size& size) {
  GLuint texture = gl::GLTestHelper::CreateTexture(GL_TEXTURE_2D);
  std::unique_ptr<uint8_t[]> pixels(new uint8_t[gfx::BufferSizeForBufferFormat(
      size, gfx::BufferFormat::RGBA_8888)]);

  const uint8_t texture_color[] = {0, 0, 0xff, 0xff};

  gl::GLImageTestSupport::SetBufferDataToColor(
      size.width(), size.height(),
      static_cast<int>(gfx::RowSizeForBufferFormat(size.width(),
                                              gfx::BufferFormat::RGBA_8888, 0)),
      0, gfx::BufferFormat::RGBA_8888, texture_color, pixels.get());
  glBindTexture(GL_TEXTURE_2D, texture);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, size.width(), size.height(), 0,
               GL_RGBA, GL_UNSIGNED_BYTE, pixels.get());


  EGLint eglImageAttributes[] = {EGL_WIDTH,
                               size.width(),
                               EGL_HEIGHT,
                               size.height(),
                               EGL_MATCH_FORMAT_KHR,
                               EGL_FORMAT_RGBA_8888_KHR};
  EGLDisplay display = eglGetPlatformDisplayEXT(EGL_PLATFORM_X11_KHR, surface_->GetDisplay(), eglImageAttributes);
  static_assert(sizeof(uint64_t) == sizeof(EGLClientBuffer), "");
  uint64_t content_texture_large = texture;
  EGLClientBuffer client_buffer;
  std::memcpy(&client_buffer, &content_texture_large, sizeof(uint64_t));
  /*EGLImageKHR image =*/
    eglCreateImageKHR(display, EGL_NO_CONTEXT,
                      EGL_GL_TEXTURE_2D_KHR,
                      client_buffer, eglImageAttributes);

  CHECK(glGetError() == GL_NO_ERROR);
  // CHECK(eglGetError() == EGL_SUCCESS);

  GLuint texture2 = 0;
  // glGenTextures(1, &texture2);
  // glBindTexture(GL_TEXTURE_EXTERNAL_OES, texture2);

  // // Attach the EGL Image to the same texture
  // glEGLImageTargetTexture2DOES(GL_TEXTURE_EXTERNAL_OES, image);
  // glTexParameterf(GL_TEXTURE_EXTERNAL_OES, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  // glTexParameterf(GL_TEXTURE_EXTERNAL_OES, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

  // CHECK(glGetError() == GL_NO_ERROR);
  // CHECK(eglGetError() == EGL_SUCCESS);

  return texture2;
}

GLuint GlTestEnvironment::GetFrameBufferForTesting() {
  return frame_buffer_;
}

}  // namespace vr
