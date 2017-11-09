// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/ozone/platform/drm/gpu/gbm_surfaceless.h"

#include <utility>

#include "base/bind.h"
#include "base/logging.h"
#include "base/memory/ptr_util.h"
#include "base/task_scheduler/post_task.h"
#include "base/trace_event/trace_event.h"
#include "ui/ozone/common/egl_util.h"
#include "ui/ozone/platform/drm/gpu/drm_vsync_provider.h"
#include "ui/ozone/platform/drm/gpu/drm_window_proxy.h"
#include "ui/ozone/platform/drm/gpu/gbm_surface_factory.h"
#include "ui/ozone/platform/drm/gpu/scanout_buffer.h"

namespace ui {

namespace {

void WaitForFence(EGLDisplay display, EGLSyncKHR fence) {
  eglClientWaitSyncKHR(display, fence, EGL_SYNC_FLUSH_COMMANDS_BIT_KHR,
                       EGL_FOREVER_KHR);
  eglDestroySyncKHR(display, fence);
}

void DestroyFence(EGLDisplay display, EGLSyncKHR fence) {
  eglDestroySyncKHR(display, fence);
}

}  // namespace

GbmSurfaceless::GbmSurfaceless(GbmSurfaceFactory* surface_factory,
                               std::unique_ptr<DrmWindowProxy> window,
                               gfx::AcceleratedWidget widget)
    : SurfacelessEGL(gfx::Size()),
      surface_factory_(surface_factory),
      window_(std::move(window)),
      widget_(widget),
      has_implicit_external_sync_(
          HasEGLExtension("EGL_ARM_implicit_external_sync")),
      has_native_fence_sync_(HasEGLExtension("EGL_ANDROID_native_fence_sync")),
      weak_factory_(this) {
  surface_factory_->RegisterSurface(window_->widget(), this);
  unsubmitted_frames_.push_back(std::make_unique<PendingFrame>());
}

void GbmSurfaceless::QueueOverlayPlane(const OverlayPlane& plane) {
  is_on_external_drm_device_ = plane.buffer->RequiresGlFinish();
  planes_.push_back(plane);
}

bool GbmSurfaceless::Initialize(gl::GLSurfaceFormat format) {
  if (!SurfacelessEGL::Initialize(format))
    return false;
  vsync_provider_ = std::make_unique<DrmVSyncProvider>(window_.get());
  if (!vsync_provider_)
    return false;
  return true;
}

gfx::SwapResult GbmSurfaceless::SwapBuffers() {
  NOTREACHED();
  return gfx::SwapResult::SWAP_FAILED;
}

bool GbmSurfaceless::ScheduleOverlayPlane(int z_order,
                                          gfx::OverlayTransform transform,
                                          gl::GLImage* image,
                                          const gfx::Rect& bounds_rect,
                                          const gfx::RectF& crop_rect) {
  unsubmitted_frames_.back()->overlays.push_back(
      gl::GLSurfaceOverlay(z_order, transform, image, bounds_rect, crop_rect));
  return true;
}

bool GbmSurfaceless::IsOffscreen() {
  return false;
}

gfx::VSyncProvider* GbmSurfaceless::GetVSyncProvider() {
  return vsync_provider_.get();
}

bool GbmSurfaceless::SupportsAsyncSwap() {
  return true;
}

bool GbmSurfaceless::SupportsPostSubBuffer() {
  return true;
}

gfx::SwapResult GbmSurfaceless::PostSubBuffer(int x,
                                              int y,
                                              int width,
                                              int height) {
  // The actual sub buffer handling is handled at higher layers.
  NOTREACHED();
  return gfx::SwapResult::SWAP_FAILED;
}

void GbmSurfaceless::SwapBuffersAsync(const SwapCompletionCallback& callback) {
  TRACE_EVENT0("drm", "GbmSurfaceless::SwapBuffersAsync");
  // If last swap failed, don't try to schedule new ones.
  if (!last_swap_buffers_result_) {
    callback.Run(gfx::SwapResult::SWAP_FAILED);
    return;
  }

  // TODO: the following should be replaced by a per surface flush as it gets
  // implemented in GL drivers.
  // We need to create the fence before flushing, since if we end up using
  // a native fence, the fence sync fd is only available after a flush.
  EGLSyncKHR fence = EGL_NO_SYNC_KHR;
  if (!rely_on_implicit_sync_ || is_on_external_drm_device_) {
    fence = InsertFence();
    if (!fence) {
      callback.Run(gfx::SwapResult::SWAP_FAILED);
      return;
    }
  }

  // TODO(dcastagna): remove glFlush since eglImageFlushExternalEXT called on
  // the image should be enough (crbug.com/720045).
  glFlush();
  unsubmitted_frames_.back()->Flush();

  SwapCompletionCallback surface_swap_callback = base::Bind(
      &GbmSurfaceless::SwapCompleted, weak_factory_.GetWeakPtr(), callback);

  PendingFrame* frame = unsubmitted_frames_.back().get();
  frame->callback = surface_swap_callback;
  unsubmitted_frames_.push_back(std::make_unique<PendingFrame>());

  // TODO(dcastagna): Remove the following workaround once we get explicit sync
  // on Intel.
  // We can not rely on implicit sync on external devices (crbug.com/692508).
  // NOTE: When on internal devices, |is_on_external_drm_device_| is set to true
  // by default conservatively, and it is correctly computed after the first
  // plane is enqueued in QueueOverlayPlane, that is called from
  // GbmSurfaceless::SubmitFrame.
  // This means |is_on_external_drm_device_| could be incorrectly set to true
  // the first time we're testing it.
  if (rely_on_implicit_sync_ && !is_on_external_drm_device_) {
    frame->render_wait_task = base::BindOnce(&base::DoNothing);
    frame->no_render_wait_task = base::BindOnce(&base::DoNothing);
  } else {
    frame->render_wait_task =
        base::BindOnce(&WaitForFence, GetDisplay(), fence);
    frame->no_render_wait_task =
        base::BindOnce(&DestroyFence, GetDisplay(), fence);
  }

  if (has_native_fence_sync_ && fence != EGL_NO_SYNC_KHR) {
    int render_fence_fd = eglDupNativeFenceFDANDROID(GetDisplay(), fence);
    DCHECK_NE(render_fence_fd, EGL_NO_NATIVE_FENCE_FD_ANDROID);
    frame->render_fence_fd = base::ScopedFD(render_fence_fd);
  }

  frame->ready = true;

  SubmitFrame();
}

void GbmSurfaceless::PostSubBufferAsync(
    int x,
    int y,
    int width,
    int height,
    const SwapCompletionCallback& callback) {
  // The actual sub buffer handling is handled at higher layers.
  SwapBuffersAsync(callback);
}

EGLConfig GbmSurfaceless::GetConfig() {
  if (!config_) {
    EGLint config_attribs[] = {EGL_BUFFER_SIZE,
                               32,
                               EGL_ALPHA_SIZE,
                               8,
                               EGL_BLUE_SIZE,
                               8,
                               EGL_GREEN_SIZE,
                               8,
                               EGL_RED_SIZE,
                               8,
                               EGL_RENDERABLE_TYPE,
                               EGL_OPENGL_ES2_BIT,
                               EGL_SURFACE_TYPE,
                               EGL_DONT_CARE,
                               EGL_NONE};
    config_ = ChooseEGLConfig(GetDisplay(), config_attribs);
  }
  return config_;
}

void GbmSurfaceless::SetRelyOnImplicitSync() {
  rely_on_implicit_sync_ = true;
}

GbmSurfaceless::~GbmSurfaceless() {
  Destroy();  // The EGL surface must be destroyed before SurfaceOzone.
  surface_factory_->UnregisterSurface(window_->widget());
}

GbmSurfaceless::PendingFrame::PendingFrame() {}

GbmSurfaceless::PendingFrame::~PendingFrame() {}

bool GbmSurfaceless::PendingFrame::ScheduleOverlayPlanes(
    gfx::AcceleratedWidget widget) {
  for (const auto& overlay : overlays)
    if (!overlay.ScheduleOverlayPlane(widget))
      return false;
  return true;
}

void GbmSurfaceless::PendingFrame::Flush() {
  for (const auto& overlay : overlays)
    overlay.Flush();
}

void GbmSurfaceless::SubmitFrame() {
  DCHECK(!unsubmitted_frames_.empty());

  if (unsubmitted_frames_.front()->ready && !swap_buffers_pending_) {
    std::unique_ptr<PendingFrame> frame(std::move(unsubmitted_frames_.front()));
    unsubmitted_frames_.erase(unsubmitted_frames_.begin());
    swap_buffers_pending_ = true;

    if (!frame->ScheduleOverlayPlanes(widget_)) {
      // |callback| is a wrapper for SwapCompleted(). Call it to properly
      // propagate the failed state.
      frame->callback.Run(gfx::SwapResult::SWAP_FAILED);
      return;
    }

    window_->SchedulePageFlip(planes_, std::move(frame->render_wait_task),
                              std::move(frame->no_render_wait_task),
                              std::move(frame->render_fence_fd),
                              frame->callback);
    planes_.clear();
  }
}

EGLSyncKHR GbmSurfaceless::InsertFence() {
  const EGLint attrib_list[] = {EGL_SYNC_CONDITION_KHR,
                                EGL_SYNC_PRIOR_COMMANDS_IMPLICIT_EXTERNAL_ARM,
                                EGL_NONE};
  EGLSyncKHR fence = EGL_NO_SYNC_KHR;

  if (has_native_fence_sync_) {
    fence = eglCreateSyncKHR(GetDisplay(), EGL_SYNC_NATIVE_FENCE_ANDROID, NULL);
  } else {
    fence = eglCreateSyncKHR(GetDisplay(), EGL_SYNC_FENCE_KHR,
                             has_implicit_external_sync_ ? attrib_list : NULL);
  }

  return fence;
}

void GbmSurfaceless::SwapCompleted(const SwapCompletionCallback& callback,
                                   gfx::SwapResult result) {
  callback.Run(result);
  swap_buffers_pending_ = false;
  if (result == gfx::SwapResult::SWAP_FAILED) {
    last_swap_buffers_result_ = false;
    return;
  }

  SubmitFrame();
}

}  // namespace ui
