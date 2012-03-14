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

namespace {
EGLDisplay g_display;
EGLNativeDisplayType g_native_display;
}

GLSurfaceDRM::GLSurfaceDRM()
  : size_(Size(1,1)) {
}

GLSurfaceDRM::GLSurfaceDRM(const Size& size)
    : size_(size) {
  if (size != Size(1, 1))
    LOG(ERROR) << "Attempted to create bad-sized GLSurfaceDRM.";
}

GLSurfaceDRM::~GLSurfaceDRM() {
  Destroy();
}

EGLDisplay GLSurfaceDRM::GetDisplay() {
  return g_display;
}

// static
EGLDisplay GLSurfaceDRM::GetHardwareDisplay() {
  return g_display;
}

// static
EGLNativeDisplayType GLSurfaceDRM::GetNativeDisplay() {
  return g_native_display;
}

EGLConfig GLSurfaceDRM::GetConfig() {
  return NULL;
}

bool GLSurfaceDRM::Initialize() {
  return true;
}

void GLSurfaceDRM::Destroy() {
}

bool GLSurfaceDRM::IsOffscreen() {
  return true;
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

  if (size_ == Size(1,1)) {
    size_ = size;
    return true;
  }
  else
    LOG(ERROR) << "Attempted to resize a GLSurfaceDRM.";

  return false;
}

EGLSurface GLSurfaceDRM::GetHandle() {
  return EGL_NO_SURFACE;
}

bool GLSurfaceDRM::InitializeOneOff() {
  static bool initialized = false;

  if (initialized)
    return true;

  g_native_display = aura::RootWindowHostDRM::GetInstance()->GetDisplay();
  g_display = eglGetDisplay(g_native_display);
  if (!g_display) {
    LOG(ERROR) << "eglGetDisplay failed with error " << GetLastEGLErrorString();
    return false;
  }

  if (!eglInitialize(g_display, NULL, NULL)) {
    LOG(ERROR) << "eglInitialize failed with error " << GetLastEGLErrorString();
    return false;
  }

  initialized = true;

  return true;
}

class NativeViewGLSurfaceDRM : public GLSurfaceDRM {
 public:
  NativeViewGLSurfaceDRM(AcceleratedWidget window);
  virtual ~NativeViewGLSurfaceDRM();

  // Implement GLSurface.
  virtual bool Initialize() OVERRIDE;
  virtual void Destroy() OVERRIDE;
  virtual bool IsOffscreen() OVERRIDE;
  virtual bool SwapBuffers() OVERRIDE;
  virtual bool OnMakeCurrent(GLContext* context) OVERRIDE;

 private:
  AcceleratedWidget kms_;
  EGLNativePixmapType pixmap_[2];
  EGLImageKHR image_[2];
  uint32_t fb_id_[2];
  GLuint color_rb_[2];
  GLuint fb_;
  int current_;
  int fd_;

  DISALLOW_COPY_AND_ASSIGN(NativeViewGLSurfaceDRM);
};

NativeViewGLSurfaceDRM::NativeViewGLSurfaceDRM(AcceleratedWidget window) {
  current_ = 0;
  kms_ = window;
  for (int i = 0; i < 1; i++)
    image_[i] = NULL;
  fd_ = gbm_device_get_fd(g_native_display);
}

NativeViewGLSurfaceDRM::~NativeViewGLSurfaceDRM() {
  Destroy();
}

bool NativeViewGLSurfaceDRM::Initialize() {
  return true;
}

void NativeViewGLSurfaceDRM::Destroy() {
  int i;
  if (image_[0]) {
    for (i = 0; i < 1; i++) {
      if (!eglDestroyImageKHR(GetDisplay(), image_[i])) {
	LOG(ERROR) << "eglDestroyImageKHR failed with error "
		   << GetLastEGLErrorString();
      }
      gbm_bo_destroy(pixmap_[i]);
      drmModeRmFB(fd_, fb_id_[i]);
      image_[i] = NULL;
    }
  }
}

bool NativeViewGLSurfaceDRM::IsOffscreen() {
  return false;
}

void page_flip_handler(int fd, unsigned int frame, unsigned int sec,
		       unsigned int usec, void *data) {}

bool NativeViewGLSurfaceDRM::SwapBuffers() {
  drmEventContext evctx;
  GLuint fb;
  fd_set rfds;
  int ret;

  glGetIntegerv(GL_FRAMEBUFFER_BINDING, (GLint *)&fb);

  if (fb != fb_) {
    // There is some sneaky code in LayerRendererChromium::UseRendererSurface
    // that binds the system FBO. For DRM, we need to bind this fb.
    LOG(ERROR) << "Hmm, framebuffer binding has changed";
    glBindFramebufferEXT(GL_FRAMEBUFFER, fb_);
    if (glCheckFramebufferStatusEXT(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
      LOG(ERROR) << "framebuffer not complete";
      return false;
    }
  }

  glFlush();

  ret = drmModePageFlip(fd_, kms_->encoder->crtc_id, fb_id_[current_],
                        DRM_MODE_PAGE_FLIP_EVENT, 0);
  CHECK(!ret);

  current_ ^= 1;

  glFramebufferRenderbufferEXT(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
			       GL_RENDERBUFFER, color_rb_[current_]);

  if (glCheckFramebufferStatusEXT(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
    LOG(ERROR) << "framebuffer not complete";
    return false;
  }

  FD_ZERO(&rfds);
  FD_SET(fd_, &rfds);

  while (select(fd_ + 1, &rfds, NULL, NULL, NULL) == -1);

  memset(&evctx, 0, sizeof evctx);
  evctx.version = DRM_EVENT_CONTEXT_VERSION;
  evctx.page_flip_handler = page_flip_handler;

  drmHandleEvent(fd_, &evctx);

  return true;
}

bool NativeViewGLSurfaceDRM::OnMakeCurrent(GLContext* context) {
  if (image_[0])
    return true;

  Resize(Size(1280, 800));

  int ret;

  glGenFramebuffersEXT(1, &fb_);
  glBindFramebufferEXT(GL_FRAMEBUFFER, fb_);

  glGenRenderbuffersEXT(2, color_rb_);
  for (int i = 0; i < 2; i++) {
    uint32_t handle, stride;

    glBindRenderbufferEXT(GL_RENDERBUFFER, color_rb_[i]);

    pixmap_[i]  = gbm_bo_create(g_native_display,
				size_.width(), size_.height(),
				GBM_BO_FORMAT_XRGB8888,
				GBM_BO_USE_SCANOUT | GBM_BO_USE_RENDERING);
    CHECK(pixmap_[i]);

    handle = gbm_bo_get_handle(pixmap_[i]).u32;
    stride = gbm_bo_get_pitch(pixmap_[i]);
    ret = drmModeAddFB(fd_, size_.width(), size_.height(),
                       24, 32, stride, handle, &fb_id_[i]);
    CHECK(!ret);

    image_[i] = eglCreateImageKHR(GetDisplay(), NULL, EGL_NATIVE_PIXMAP_KHR,
				  pixmap_[i], NULL);
    CHECK(image_[i]);

    glEGLImageTargetRenderbufferStorageOES(GL_RENDERBUFFER, image_[i]);

  }

  glFramebufferRenderbufferEXT(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
				 GL_RENDERBUFFER, color_rb_[current_]);
  if (glCheckFramebufferStatusEXT(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
    LOG(ERROR) << "framebuffer not complete";
    return false;
  }

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
