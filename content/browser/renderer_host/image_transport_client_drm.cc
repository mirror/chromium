// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/renderer_host/image_transport_client.h"

#include "base/debug/trace_event.h"
#include "base/lazy_instance.h"
#include "base/memory/scoped_ptr.h"
#include "content/browser/renderer_host/image_transport_factory.h"
#include "third_party/angle/include/EGL/egl.h"
#include "third_party/angle/include/EGL/eglext.h"
#include "ui/gfx/gl/gl_bindings.h"
#include "ui/gfx/gl/gl_implementation.h"
#include "ui/gfx/gl/gl_surface_drm.h"
#include "ui/gfx/gl/scoped_make_current.h"
#include "ui/gfx/size.h"

namespace {

GLuint CreateTexture() {
  GLuint texture;
  glGenTextures(1, &texture);
  glBindTexture(GL_TEXTURE_2D, texture);
  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  return texture;
}

class ImageTransportClientDRM : public ImageTransportClient {
 public:
  ImageTransportClientDRM(ImageTransportFactory* factory, const gfx::Size& size)
      : ImageTransportClient(true, size),
        factory_(factory),
        image_(NULL) {
  }

  virtual ~ImageTransportClientDRM() {
    scoped_ptr<gfx::ScopedMakeCurrent> bind(factory_->GetScopedMakeCurrent());
    if (image_)
      eglDestroyImageKHR(gfx::GLSurfaceDRM::GetHardwareDisplay(), image_);
    unsigned int texture = texture_id();
    if (texture)
      glDeleteTextures(1, &texture);
    glFlush();
  }

  virtual bool Initialize(uint64* surface_handle) {
    scoped_ptr<gfx::ScopedMakeCurrent> bind(factory_->GetScopedMakeCurrent());
    image_ = eglCreateImageKHR(
        gfx::GLSurfaceDRM::GetHardwareDisplay(), EGL_NO_CONTEXT,
        EGL_NATIVE_PIXMAP_KHR, reinterpret_cast<void*>(*surface_handle), NULL);
    CHECK(image_);
    set_texture_id(CreateTexture());
    glBindTexture(GL_TEXTURE_2D, texture_id());
    glEGLImageTargetTexture2DOES(GL_TEXTURE_2D, image_);
    glFlush();
    return true;
  }

  virtual void Update() { }
  virtual TransportDIB::Handle Handle() const {
    return TransportDIB::DefaultHandleValue();
  }

 private:
  ImageTransportFactory* factory_;
  EGLImageKHR image_;
};

}  // anonymous namespace

ImageTransportClient* ImageTransportClient::Create(
    ImageTransportFactory* factory,
    const gfx::Size& size) {
  switch (gfx::GetGLImplementation()) {
    case gfx::kGLImplementationEGLGLES2:
      return new ImageTransportClientDRM(factory, size);
    default:
      NOTREACHED();
      return NULL;
  }
}
