// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/gl/gl_image_egl.h"

#include "ui/gl/egl_util.h"
#include "ui/gl/gl_surface_egl.h"

#define FOURCC(a, b, c, d)                                        \
  ((static_cast<uint32_t>(a)) | (static_cast<uint32_t>(b) << 8) | \
   (static_cast<uint32_t>(c) << 16) | (static_cast<uint32_t>(d) << 24))

#define DRM_FORMAT_RGB565 FOURCC('R', 'G', '1', '6')
#define DRM_FORMAT_ARGB8888 FOURCC('A', 'R', '2', '4')
#define DRM_FORMAT_ABGR8888 FOURCC('A', 'B', '2', '4')
#define DRM_FORMAT_XRGB8888 FOURCC('X', 'R', '2', '4')
#define DRM_FORMAT_XBGR8888 FOURCC('X', 'B', '2', '4')
#define DRM_FORMAT_YVU420 FOURCC('Y', 'V', '1', '2')

#define DRM_FORMAT_MOD_INVALID ((1ULL << 56) - 1)

namespace gl {

GLImageEGL::GLImageEGL(const gfx::Size& size)
    : egl_image_(EGL_NO_IMAGE_KHR), size_(size) {}

GLImageEGL::~GLImageEGL() {
  DCHECK(thread_checker_.CalledOnValidThread());
  if (egl_image_ != EGL_NO_IMAGE_KHR) {
    EGLBoolean result =
        eglDestroyImageKHR(GLSurfaceEGL::GetHardwareDisplay(), egl_image_);
    if (result == EGL_FALSE) {
      DLOG(ERROR) << "Error destroying EGLImage: "
                  << ui::GetLastEGLErrorString();
    }
  }
}

bool GLImageEGL::Initialize(EGLenum target,
                            EGLClientBuffer buffer,
                            const EGLint* attrs) {
  DCHECK(thread_checker_.CalledOnValidThread());
  DCHECK_EQ(EGL_NO_IMAGE_KHR, egl_image_);
  egl_image_ = eglCreateImageKHR(GLSurfaceEGL::GetHardwareDisplay(),
                                 EGL_NO_CONTEXT, target, buffer, attrs);
  if (egl_image_ == EGL_NO_IMAGE_KHR) {
    LOG(ERROR) << "Error creating EGLImage: " << ui::GetLastEGLErrorString();
    return false;
  }

  return true;
}

std::vector<gfx::BufferAttribute> GLImageEGL::QueryDmaBufBufferAttributes() {
  bool has_dma_buf_import_modifier = gl::GLSurfaceEGL::HasEGLExtension(
      "EGL_EXT_image_dma_buf_import_modifiers");
  if (!has_dma_buf_import_modifier) {
    std::vector<gfx::BufferAttribute> defaults = {
        {DRM_FORMAT_RGB565, DRM_FORMAT_MOD_INVALID},
        {DRM_FORMAT_XBGR8888, DRM_FORMAT_MOD_INVALID},
        {DRM_FORMAT_ABGR8888, DRM_FORMAT_MOD_INVALID},
        {DRM_FORMAT_XRGB8888, DRM_FORMAT_MOD_INVALID},
        {DRM_FORMAT_ARGB8888, DRM_FORMAT_MOD_INVALID},
        {DRM_FORMAT_YVU420, DRM_FORMAT_MOD_INVALID}};
    return defaults;
  }

  EGLDisplay dpy = GLSurfaceEGL::GetHardwareDisplay();
  std::vector<gfx::BufferAttribute> supported;
  EGLint *drm_formats, num_formats;

  eglQueryDmaBufFormatsEXT(dpy, 0, NULL, &num_formats);
  drm_formats = new EGLint[num_formats];
  eglQueryDmaBufFormatsEXT(dpy, num_formats, drm_formats, &num_formats);

  for (int i = 0; i < num_formats; i++) {
    EGLuint64KHR* drm_modifiers;
    EGLint num_modifiers;

    eglQueryDmaBufModifiersEXT(dpy, drm_formats[i], 0, NULL, NULL,
                               &num_modifiers);
    drm_modifiers = new EGLuint64KHR[num_modifiers];
    eglQueryDmaBufModifiersEXT(dpy, drm_formats[i], num_modifiers,
                               drm_modifiers, NULL, &num_modifiers);

    if (num_modifiers == 0)
      supported.push_back(
          gfx::BufferAttribute(drm_formats[i], DRM_FORMAT_MOD_INVALID));
    for (int j = 0; j < num_modifiers; j++)
      supported.push_back(
          gfx::BufferAttribute(drm_formats[i], drm_modifiers[j]));
    delete[] drm_modifiers;
  }
  delete[] drm_formats;
  return supported;
}

gfx::Size GLImageEGL::GetSize() {
  return size_;
}

unsigned GLImageEGL::GetInternalFormat() { return GL_RGBA; }

bool GLImageEGL::BindTexImage(unsigned target) {
  DCHECK(thread_checker_.CalledOnValidThread());
  if (egl_image_ == EGL_NO_IMAGE_KHR)
    return false;

  glEGLImageTargetTexture2DOES(target, egl_image_);
  DCHECK_EQ(static_cast<GLenum>(GL_NO_ERROR), glGetError());
  return true;
}

bool GLImageEGL::CopyTexImage(unsigned target) {
  return false;
}

bool GLImageEGL::CopyTexSubImage(unsigned target,
                                 const gfx::Point& offset,
                                 const gfx::Rect& rect) {
  return false;
}

bool GLImageEGL::ScheduleOverlayPlane(gfx::AcceleratedWidget widget,
                                      int z_order,
                                      gfx::OverlayTransform transform,
                                      const gfx::Rect& bounds_rect,
                                      const gfx::RectF& crop_rect) {
  return false;
}

}  // namespace gl
