// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/ozone/demo/surfaceless_gl_renderer.h"

#include "ui/gl/gl_bindings.h"
#include "ui/gl/gl_image.h"

namespace ui {

SurfacelessGlRenderer::BufferWrapper::BufferWrapper()
    : widget_(gfx::kNullAcceleratedWidget), gl_fb_(0), gl_tex_(0) {
}

SurfacelessGlRenderer::BufferWrapper::~BufferWrapper() {
  if (gl_fb_)
    glDeleteFramebuffersEXT(1, &gl_fb_);

  if (gl_tex_) {
    image_->ReleaseTexImage(GL_TEXTURE_2D);
    glDeleteTextures(1, &gl_tex_);
    image_->Destroy(true);
  }
}

bool SurfacelessGlRenderer::BufferWrapper::Initialize(
    GpuMemoryBufferFactoryOzoneNativeBuffer* buffer_factory,
    gfx::AcceleratedWidget widget,
    const gfx::Size& size) {
  glGenFramebuffersEXT(1, &gl_fb_);
  glGenTextures(1, &gl_tex_);

  static int buffer_id_generator = 1;
  int id = buffer_id_generator++;

  buffer_factory->CreateGpuMemoryBuffer(
      id, size, gfx::GpuMemoryBuffer::RGBX_8888, gfx::GpuMemoryBuffer::SCANOUT,
      1, widget);
  image_ = buffer_factory->CreateImageForGpuMemoryBuffer(
      id, size, gfx::GpuMemoryBuffer::RGBX_8888, GL_RGB, 1);
  // Now that we have a reference to |image_|; we can just remove it from the
  // factory mapping.
  buffer_factory->DestroyGpuMemoryBuffer(id, widget);

  if (!image_) {
    LOG(ERROR) << "Failed to create GL image";
    return false;
  }

  glBindFramebufferEXT(GL_FRAMEBUFFER, gl_fb_);
  glBindTexture(GL_TEXTURE_2D, gl_tex_);
  image_->BindTexImage(GL_TEXTURE_2D);

  glFramebufferTexture2DEXT(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D,
                            gl_tex_, 0);
  if (glCheckFramebufferStatusEXT(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
    LOG(ERROR) << "Failed to create framebuffer "
               << glCheckFramebufferStatusEXT(GL_FRAMEBUFFER);
    return false;
  }

  widget_ = widget;
  size_ = size;

  return true;
}

void SurfacelessGlRenderer::BufferWrapper::BindFramebuffer() {
  glBindFramebufferEXT(GL_FRAMEBUFFER, gl_fb_);
}

void SurfacelessGlRenderer::BufferWrapper::SchedulePlane() {
  image_->ScheduleOverlayPlane(widget_, 0, gfx::OVERLAY_TRANSFORM_NONE,
                               gfx::Rect(size_), gfx::RectF(0, 0, 1, 1));
}

SurfacelessGlRenderer::SurfacelessGlRenderer(gfx::AcceleratedWidget widget,
                                             const gfx::Size& size)
    : GlRenderer(widget, size), back_buffer_(0) {
}

SurfacelessGlRenderer::~SurfacelessGlRenderer() {
}

bool SurfacelessGlRenderer::Initialize() {
  if (!GlRenderer::Initialize())
    return false;

  for (size_t i = 0; i < arraysize(buffers_); ++i)
    if (!buffers_[i].Initialize(&buffer_factory_, widget_, size_))
      return false;

  return true;
}

void SurfacelessGlRenderer::RenderFrame() {
  buffers_[back_buffer_].BindFramebuffer();
  buffers_[back_buffer_].SchedulePlane();
  back_buffer_ ^= 1;

  GlRenderer::RenderFrame();
}

}  // namespace ui
