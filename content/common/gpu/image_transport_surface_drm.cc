// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/common/gpu/image_transport_surface.h"

#include <gbm.h>

#include <map>
#include <vector>

// Note: these must be included before anything that includes gl_bindings.h
// They're effectively standard library headers.
#include "third_party/angle/include/EGL/egl.h"
#include "third_party/angle/include/EGL/eglext.h"

#include "base/bind.h"
#include "base/memory/weak_ptr.h"
#include "base/debug/trace_event.h"
#include "content/common/gpu/gpu_channel.h"
#include "content/common/gpu/gpu_channel_manager.h"
#include "content/common/gpu/gpu_command_buffer_stub.h"
#include "content/common/gpu/gpu_messages.h"
#include "content/common/gpu/texture_image_transport_surface.h"
#include "gpu/command_buffer/service/gpu_scheduler.h"
#include "ui/gfx/gl/gl_context.h"
#include "ui/gfx/gl/gl_bindings.h"
#include "ui/gfx/gl/gl_implementation.h"
#include "ui/gfx/gl/gl_surface_drm.h"
#include "ui/gfx/rect.h"

namespace {

// The GL context associated with the surface must be current when
// an instance is created or destroyed.
class DRMAcceleratedSurface : public base::RefCounted<DRMAcceleratedSurface> {
 public:
  explicit DRMAcceleratedSurface(const gfx::Size& size);
  const gfx::Size& size() const { return size_; }
  EGLNativePixmapType pixmap() const { return pixmap_; }
  uint32 texture() const { return texture_; }

 private:
  ~DRMAcceleratedSurface();

  gfx::Size size_;
  void* image_;
  EGLNativePixmapType pixmap_;
  uint32 texture_;

  friend class base::RefCounted<DRMAcceleratedSurface>;
  DISALLOW_COPY_AND_ASSIGN(DRMAcceleratedSurface);
};

// We are backed by an Pbuffer offscreen surface for the purposes of creating a
// context, but use FBOs to render to X Pixmap backed EGLImages.
class DRMImageTransportSurface
    : public ImageTransportSurface,
      public gfx::GLSurfaceDRM,
      public base::SupportsWeakPtr<DRMImageTransportSurface> {
 public:
  DRMImageTransportSurface(GpuChannelManager* manager,
                           GpuCommandBufferStub* stub);

  // gfx::GLSurface implementation
  virtual bool Initialize() OVERRIDE;
  virtual void Destroy() OVERRIDE;
  virtual bool IsOffscreen() OVERRIDE;
  virtual bool SwapBuffers() OVERRIDE;
  virtual bool PostSubBuffer(int x, int y, int width, int height) OVERRIDE;
  virtual std::string GetExtensions() OVERRIDE;
  virtual gfx::Size GetSize() OVERRIDE;
  virtual bool OnMakeCurrent(gfx::GLContext* context) OVERRIDE;
  virtual unsigned int GetBackingFrameBufferObject() OVERRIDE;
  virtual void SetVisible(bool visible) OVERRIDE;

 protected:
  // ImageTransportSurface implementation
  virtual void OnNewSurfaceACK(
      uint64 surface_handle, TransportDIB::Handle shm_handle) OVERRIDE;
  virtual void OnBuffersSwappedACK() OVERRIDE;
  virtual void OnPostSubBufferACK() OVERRIDE;
  virtual void OnResizeViewACK() OVERRIDE;
  virtual void OnResize(gfx::Size size) OVERRIDE;

 private:
  virtual ~DRMImageTransportSurface() OVERRIDE;
  void ReleaseSurface(scoped_refptr<DRMAcceleratedSurface>* surface);
  void SendBuffersSwapped();
  void SendPostSubBuffer(int x, int y, int width, int height);
  void GetRegionsToCopy(const gfx::Rect& new_damage_rect,
                        std::vector<gfx::Rect>* regions);

  uint32 fbo_id_;

  scoped_refptr<DRMAcceleratedSurface> back_surface_;
  scoped_refptr<DRMAcceleratedSurface> front_surface_;
  gfx::Rect previous_damage_rect_;

  // Whether or not we've successfully made the surface current once.
  bool made_current_;

  scoped_ptr<ImageTransportHelper> helper_;

  DISALLOW_COPY_AND_ASSIGN(DRMImageTransportSurface);
};

DRMAcceleratedSurface::DRMAcceleratedSurface(const gfx::Size& size)
    : size_(size), texture_(0) {
  gbm_device* dpy = gfx::GLSurfaceDRM::GetNativeDisplay();
  EGLDisplay edpy = gfx::GLSurfaceDRM::GetHardwareDisplay();

  pixmap_  = gbm_bo_create(dpy, size_.width(), size_.height(),
                           GBM_BO_FORMAT_XRGB8888,
                           GBM_BO_USE_SCANOUT | GBM_BO_USE_RENDERING);

  image_ = eglCreateImageKHR(edpy, EGL_NO_CONTEXT,
                             EGL_NATIVE_PIXMAP_KHR, pixmap_, NULL);

  glGenTextures(1, &texture_);

  GLint current_texture = 0;
  glGetIntegerv(GL_TEXTURE_BINDING_2D, &current_texture);

  glBindTexture(GL_TEXTURE_2D, texture_);
  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  glEGLImageTargetTexture2DOES(GL_TEXTURE_2D, image_);

  glBindTexture(GL_TEXTURE_2D, current_texture);
}

DRMAcceleratedSurface::~DRMAcceleratedSurface() {
  glDeleteTextures(1, &texture_);
  eglDestroyImageKHR(gfx::GLSurfaceDRM::GetHardwareDisplay(), image_);
  gbm_bo_destroy(pixmap_);
}

DRMImageTransportSurface::DRMImageTransportSurface(
    GpuChannelManager* manager,
    GpuCommandBufferStub* stub)
    : gfx::GLSurfaceDRM(gfx::Size(1, 1)),
      fbo_id_(0),
      made_current_(false) {
  helper_.reset(new ImageTransportHelper(this,
                                         manager,
                                         stub,
                                         gfx::kNullPluginWindow));
}

DRMImageTransportSurface::~DRMImageTransportSurface() {
  Destroy();
}

bool DRMImageTransportSurface::Initialize() {
  if (!helper_->Initialize())
    return false;
  return gfx::GLSurfaceDRM::Initialize();
}

void DRMImageTransportSurface::Destroy() {
  if (back_surface_.get())
    ReleaseSurface(&back_surface_);
  if (front_surface_.get())
    ReleaseSurface(&front_surface_);

  helper_->Destroy();
  gfx::GLSurfaceDRM::Destroy();
}

// Make sure that buffer swaps occur for the surface, so we can send the data
// to the actual onscreen surface in the browser
bool DRMImageTransportSurface::IsOffscreen() {
  return false;
}

bool DRMImageTransportSurface::OnMakeCurrent(gfx::GLContext* context) {
  if (made_current_)
    return true;

  if (!context->HasExtension("EGL_KHR_image") &&
      !context->HasExtension("EGL_KHR_image_pixmap")) {
    DLOG(ERROR) << "EGLImage from X11 pixmap not supported";
    return false;
  }

  glGenFramebuffersEXT(1, &fbo_id_);
  glBindFramebufferEXT(GL_FRAMEBUFFER, fbo_id_);
  OnResize(gfx::Size(1, 1));

  GLenum status = glCheckFramebufferStatusEXT(GL_FRAMEBUFFER);
  if (status != GL_FRAMEBUFFER_COMPLETE) {
    DLOG(ERROR) << "Framebuffer incomplete.";
    return false;
  }

  made_current_ = true;
  return true;
}

unsigned int DRMImageTransportSurface::GetBackingFrameBufferObject() {
  return fbo_id_;
}

void DRMImageTransportSurface::SetVisible(bool visible) {
  if (!visible && back_surface_.get() && front_surface_.get()) {
    ReleaseSurface(&back_surface_);
  } else if (visible && !back_surface_.get() && front_surface_.get()) {
    // Leverage the OnResize hook because it does exactly what we want
    OnResize(front_surface_->size());
  }
}

void DRMImageTransportSurface::ReleaseSurface(
    scoped_refptr<DRMAcceleratedSurface>* surface) {
  if (surface->get()) {
    GpuHostMsg_AcceleratedSurfaceRelease_Params params;
    params.identifier = (uint64)((*surface)->pixmap());
    helper_->SendAcceleratedSurfaceRelease(params);
    *surface = NULL;
  }
}

void DRMImageTransportSurface::OnResize(gfx::Size size) {
  if (back_surface_.get())
    ReleaseSurface(&back_surface_);

  back_surface_ = new DRMAcceleratedSurface(size);

  GLint previous_fbo_id = 0;
  glGetIntegerv(GL_FRAMEBUFFER_BINDING, &previous_fbo_id);

  glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, fbo_id_);
  glFramebufferTexture2DEXT(GL_FRAMEBUFFER,
                            GL_COLOR_ATTACHMENT0,
                            GL_TEXTURE_2D,
                            back_surface_->texture(),
                            0);
  glFlush();

  glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, previous_fbo_id);

  GpuHostMsg_AcceleratedSurfaceNew_Params params;
  params.width = size.width();
  params.height = size.height();
  params.surface_handle = (uint64)(back_surface_->pixmap());
  helper_->SendAcceleratedSurfaceNew(params);

  helper_->SetScheduled(false);
}

bool DRMImageTransportSurface::SwapBuffers() {
  front_surface_.swap(back_surface_);
  DCHECK_NE(front_surface_.get(), static_cast<DRMAcceleratedSurface*>(NULL));
  helper_->DeferToFence(base::Bind(
      &DRMImageTransportSurface::SendBuffersSwapped,
      AsWeakPtr()));

  gfx::Size expected_size = front_surface_->size();
  if (!back_surface_.get() || back_surface_->size() != expected_size) {
    OnResize(expected_size);
  } else {
    glFramebufferTexture2DEXT(GL_FRAMEBUFFER,
                              GL_COLOR_ATTACHMENT0,
                              GL_TEXTURE_2D,
                              back_surface_->texture(),
                              0);
  }
  previous_damage_rect_ = gfx::Rect(front_surface_->size());
  return true;
}

void DRMImageTransportSurface::SendBuffersSwapped() {
  GpuHostMsg_AcceleratedSurfaceBuffersSwapped_Params params;
  params.surface_handle = (uint64)(front_surface_->pixmap());
  helper_->SendAcceleratedSurfaceBuffersSwapped(params);
  helper_->SetScheduled(false);
}

bool DRMImageTransportSurface::PostSubBuffer(
    int x, int y, int width, int height) {

  DCHECK_NE(back_surface_.get(), static_cast<DRMAcceleratedSurface*>(NULL));
  gfx::Size expected_size = back_surface_->size();
  bool surfaces_same_size = front_surface_.get() &&
      front_surface_->size() == expected_size;

  const gfx::Rect new_damage_rect = gfx::Rect(x, y, width, height);
  if (surfaces_same_size) {
    std::vector<gfx::Rect> regions_to_copy;
    GetRegionsToCopy(new_damage_rect, &regions_to_copy);

    GLint previous_texture_id = 0;
    glGetIntegerv(GL_ACTIVE_TEXTURE, &previous_texture_id);
    glFramebufferTexture2DEXT(GL_FRAMEBUFFER,
                              GL_COLOR_ATTACHMENT0,
                              GL_TEXTURE_2D,
                              front_surface_->texture(),
                              0);
    glBindTexture(GL_TEXTURE_2D, back_surface_->texture());

    for (size_t i = 0; i < regions_to_copy.size(); ++i) {
      const gfx::Rect& region_to_copy = regions_to_copy[i];
      if (!region_to_copy.IsEmpty()) {
        glCopyTexSubImage2D(GL_TEXTURE_2D, 0, region_to_copy.x(),
            region_to_copy.y(), region_to_copy.x(), region_to_copy.y(),
            region_to_copy.width(), region_to_copy.height());
      }
    }
    glBindTexture(GL_TEXTURE_2D, previous_texture_id);
  }

  front_surface_.swap(back_surface_);

  if (!surfaces_same_size) {
    DCHECK(new_damage_rect == gfx::Rect(expected_size));
    OnResize(expected_size);
  }

  helper_->DeferToFence(base::Bind(
      &DRMImageTransportSurface::SendPostSubBuffer,
      AsWeakPtr(), x, y, width, height));

  previous_damage_rect_ = new_damage_rect;

  return true;
}

void DRMImageTransportSurface::SendPostSubBuffer(
    int x, int y, int width, int height) {
  GpuHostMsg_AcceleratedSurfacePostSubBuffer_Params params;
  params.surface_handle = (uint64)(front_surface_->pixmap());
  params.x = x;
  params.y = y;
  params.width = width;
  params.height = height;

  helper_->SendAcceleratedSurfacePostSubBuffer(params);
  helper_->SetScheduled(false);
}

void DRMImageTransportSurface::GetRegionsToCopy(
    const gfx::Rect& new_damage_rect,
    std::vector<gfx::Rect>* regions) {
  DCHECK(front_surface_->size() == back_surface_->size());
  gfx::Rect intersection = previous_damage_rect_.Intersect(new_damage_rect);

  if (intersection.IsEmpty()) {
    regions->push_back(previous_damage_rect_);
    return;
  }

  // Top (above the intersection).
  regions->push_back(gfx::Rect(previous_damage_rect_.x(),
      previous_damage_rect_.y(),
      previous_damage_rect_.width(),
      intersection.y() - previous_damage_rect_.y()));

  // Left (of the intersection).
  regions->push_back(gfx::Rect(previous_damage_rect_.x(),
      intersection.y(),
      intersection.x() - previous_damage_rect_.x(),
      intersection.height()));

  // Right (of the intersection).
  regions->push_back(gfx::Rect(intersection.right(),
      intersection.y(),
      previous_damage_rect_.right() - intersection.right(),
      intersection.height()));

  // Bottom (below the intersection).
  regions->push_back(gfx::Rect(previous_damage_rect_.x(),
      intersection.bottom(),
      previous_damage_rect_.width(),
      previous_damage_rect_.bottom() - intersection.bottom()));
}

std::string DRMImageTransportSurface::GetExtensions() {
  std::string extensions = gfx::GLSurface::GetExtensions();
  extensions += extensions.empty() ? "" : " ";
  extensions += "GL_CHROMIUM_front_buffer_cached ";
  extensions += "GL_CHROMIUM_post_sub_buffer";
  return extensions;
}

gfx::Size DRMImageTransportSurface::GetSize() {
  return back_surface_->size();
}

void DRMImageTransportSurface::OnNewSurfaceACK(
    uint64 surface_handle, TransportDIB::Handle /*shm_handle*/) {
  DCHECK_EQ((uint64)(back_surface_->pixmap()), surface_handle);
  helper_->SetScheduled(true);
}

void DRMImageTransportSurface::OnBuffersSwappedACK() {
  helper_->SetScheduled(true);
}

void DRMImageTransportSurface::OnPostSubBufferACK() {
  helper_->SetScheduled(true);
}

void DRMImageTransportSurface::OnResizeViewACK() {
  NOTREACHED();
}

}  // namespace

// static
scoped_refptr<gfx::GLSurface> ImageTransportSurface::CreateSurface(
    GpuChannelManager* manager,
    GpuCommandBufferStub* stub,
    const gfx::GLSurfaceHandle& handle) {
  scoped_refptr<gfx::GLSurface> surface;
  if (!handle.handle) {
    DCHECK(handle.transport);
    if (!handle.parent_client_id) {
      switch (gfx::GetGLImplementation()) {
      case gfx::kGLImplementationEGLGLES2:
	surface = new DRMImageTransportSurface(manager, stub);
	break;
      default:
	NOTREACHED();
	return NULL;
      }
    } else {
      surface = new TextureImageTransportSurface(manager, stub, handle);
    }
  } else {
    surface = gfx::GLSurface::CreateViewGLSurface(false, handle.handle);
    if (!surface.get())
      return NULL;
    surface = new PassThroughImageTransportSurface(manager,
                                                   stub,
                                                   surface.get(),
                                                   handle.transport);
  }

  if (surface->Initialize())
    return surface;
  else
    return NULL;
}
