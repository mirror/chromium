// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/gfx/gl/gl_surface_drm.h"

#include <gbm.h>
#include <xf86drm.h>
#include <xf86drmMode.h>

#include "third_party/angle/include/EGL/egl.h"
#include "third_party/angle/include/EGL/eglext.h"

#include "base/logging.h"
#include "ui/aura/root_window_host_drm.h"
#include "ui/gfx/gl/egl_util.h"
#include "ui/gfx/gl/gl_bindings.h"
#include "ui/gfx/gl/gl_context.h"
#include "ui/gfx/gl/gl_implementation.h"

namespace gfx {

GLSurfaceDRM::GLSurfaceDRM(const Size& size)
  : size_(size),
    surface_(NULL),
    native_surface_(NULL),
    is_offscreen_(true) {
}

GLSurfaceDRM::~GLSurfaceDRM() {
  Destroy();
}

bool GLSurfaceDRM::Initialize() {
  native_surface_ = gbm_surface_create(GLSurfaceEGL::GetNativeDisplay(),
				       size_.width(), size_.height(),
				       GBM_FORMAT_XRGB8888,
				       GBM_BO_USE_RENDERING |
				       (is_offscreen_ ? 0: GBM_BO_USE_SCANOUT));
  if (!native_surface_) {
    LOG(ERROR) << "failed to create dummy gbm surface\n";
    Destroy();
    return false;
  }

  surface_ = eglCreateWindowSurface(GetDisplay(),
				    GetConfig(),
				    native_surface_,
				    NULL);
  if (!surface_) {
    LOG(ERROR) << "eglCreateWindowSurface failed with error "
               << GetLastEGLErrorString();
    Destroy();
    return false;
  }

  return true;
}

void GLSurfaceDRM::Destroy() {
  if (surface_) {
    if (!eglDestroySurface(GetDisplay(), surface_)) {
      LOG(ERROR) << "eglDestroySurface failed with error "
                 << GetLastEGLErrorString();
    }
    surface_ = NULL;
  }
  if (native_surface_) {
    gbm_surface_destroy(native_surface_);
    native_surface_ = NULL;
  }
}

bool GLSurfaceDRM::IsOffscreen() {
  return is_offscreen_;
}

bool GLSurfaceDRM::SwapBuffers() {
  LOG(ERROR) << "Attempted to call SwapBuffers on a GLSurfaceDRM.";
  return false;
}

gfx::Size GLSurfaceDRM::GetSize() {
  return size_;
}

bool GLSurfaceDRM::Resize(const gfx::Size& size) {
  if (size == size_)
    return true;

  GLContext* current_context = GLContext::GetCurrent();
  bool was_current = current_context && current_context->IsCurrent(this);
  if (was_current)
    current_context->ReleaseCurrent(this);

  Destroy();

  size_ = size;

  if (!Initialize())
    return false;

  if (was_current)
    return current_context->MakeCurrent(this);

  return true;
}

EGLSurface GLSurfaceDRM::GetHandle() {
  return surface_;
}

class NativeViewGLSurfaceDRM : public GLSurfaceDRM {
 public:
  NativeViewGLSurfaceDRM(AcceleratedWidget window);
  virtual ~NativeViewGLSurfaceDRM();
  void PageFlipHandler(int frame, unsigned int sec, unsigned int usec);

  // Implement GLSurface.
  virtual bool SwapBuffers() OVERRIDE;

 private:
  AcceleratedWidget window_;
  int fd_;
  EGLNativePixmapType current_bo_, next_bo_;
  uint32_t current_fb_id_, next_fb_id_;
  bool first_time_;

  DISALLOW_COPY_AND_ASSIGN(NativeViewGLSurfaceDRM);
};

NativeViewGLSurfaceDRM::NativeViewGLSurfaceDRM(AcceleratedWidget window)
    : GLSurfaceDRM(Size(1280, 800)),
      window_(window),
      fd_(gbm_device_get_fd(GLSurfaceEGL::GetNativeDisplay())),
      current_bo_(NULL),
      next_bo_(NULL),
      current_fb_id_(0),
      next_fb_id_(0),
      first_time_(true) {
  is_offscreen_ = false;
}

NativeViewGLSurfaceDRM::~NativeViewGLSurfaceDRM() {
  Destroy();
}

void page_flip_handler(int fd, unsigned int frame, unsigned int sec,
		       unsigned int usec, void *data) {
  NativeViewGLSurfaceDRM *surface;

  surface = static_cast<NativeViewGLSurfaceDRM *>(data);
  surface->PageFlipHandler(frame, sec, usec);
}

void NativeViewGLSurfaceDRM::PageFlipHandler(int frame,
					     unsigned int sec,
					     unsigned int usec) {
  if (current_fb_id_)
    drmModeRmFB(fd_, current_fb_id_);
  current_fb_id_ = next_fb_id_;
  next_fb_id_ = 0;

  if (current_bo_)
    gbm_surface_release_buffer(native_surface_, current_bo_);
  current_bo_ = next_bo_;
  next_bo_ = NULL;
}

bool NativeViewGLSurfaceDRM::SwapBuffers() {
  uint32_t handle, stride;
  int ret;

  if (!eglSwapBuffers(GetDisplay(), surface_)) {
    LOG(ERROR) << "eglSwapBuffers failed with error "
	       << GetLastEGLErrorString();
    return false;
  }

  if (first_time_) {
    first_time_ = false;
  } else {
    drmEventContext evctx;
    fd_set rfds;

    FD_ZERO(&rfds);
    FD_SET(fd_, &rfds);

    while (select(fd_ + 1, &rfds, NULL, NULL, NULL) == -1);

    memset(&evctx, 0, sizeof evctx);
    evctx.version = DRM_EVENT_CONTEXT_VERSION;
    evctx.page_flip_handler = page_flip_handler;

    drmHandleEvent(fd_, &evctx);
  }

  next_bo_ = gbm_surface_lock_front_buffer(native_surface_);
  if (!next_bo_) {
    LOG(ERROR) << "failed to lock front buffer";
    return false;
  }

  handle = gbm_bo_get_handle(next_bo_).u32;
  stride = gbm_bo_get_pitch(next_bo_);

  ret = drmModeAddFB(fd_,
		     size_.width(), size_.height(),
		     24, 32, stride, handle, &next_fb_id_);
  CHECK(!ret);

  ret = drmModePageFlip(fd_, window_, next_fb_id_,
			DRM_MODE_PAGE_FLIP_EVENT, this);
  CHECK(!ret);

  return true;
}

bool GLSurface::InitializeOneOffInternal() {
  if (!GLSurfaceDRM::InitializeOneOff()) {
    LOG(ERROR) << "GLSurfaceEGL::InitializeOneOff failed.";
    return false;
  }

  return true;
}

scoped_refptr<GLSurface> GLSurface::CreateViewGLSurface(
    bool software, AcceleratedWidget window) {

  CHECK(!software);

  scoped_refptr<GLSurface> surface(new NativeViewGLSurfaceDRM(window));
  if (!surface->Initialize()) {
    LOG(ERROR) << "Unable to initialize NativeViewGLSurfaceDRM\n";
    return NULL;
  }
  return surface;
}

scoped_refptr<GLSurface> GLSurface::CreateOffscreenGLSurface(
    bool software, const Size& size) {

  CHECK(!software);

  scoped_refptr<GLSurface> surface(new GLSurfaceDRM(size));
  if (!surface->Initialize()) {
    LOG(ERROR) << "Unable to initialize GLSurfaceDRM\n";
    return NULL;
  }
  return surface;
}

} // namespace gfx
