// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "gpu/ipc/service/pass_through_image_transport_surface.h"

#include "base/bind.h"
#include "base/bind_helpers.h"
#include "base/command_line.h"
#include "build/build_config.h"
#include "ui/gfx/vsync_provider.h"
#include "ui/gl/gl_context.h"
#include "ui/gl/gl_switches.h"

namespace gpu {

namespace {
// Number of swap generations before vsync is reenabled after we've stopped
// doing multiple swaps per frame.
const int kMultiWindowSwapEnableVSyncDelay = 60;

int g_current_swap_generation_ = 0;
int g_num_swaps_in_current_swap_generation_ = 0;
int g_last_multi_window_swap_generation_ = 0;

bool HasSwitch(const char switch_constant[]) {
  return base::CommandLine::ForCurrentProcess()->HasSwitch(switch_constant);
}

}  // anonymous namespace

PassThroughImageTransportSurface::PassThroughImageTransportSurface(
    base::WeakPtr<ImageTransportSurfaceDelegate> delegate,
    gl::GLSurface* surface,
    MultiWindowSwapInterval multi_window_swap_interval)
    : GLSurfaceAdapter(surface),
      is_gpu_vsync_disabled_(HasSwitch(switches::kDisableGpuVsync)),
      is_presentation_callback_enabled_(
          HasSwitch(switches::kEnablePresentationCallback)),
      delegate_(delegate),
      multi_window_swap_interval_(multi_window_swap_interval),
      weak_ptr_factory_(this) {}

bool PassThroughImageTransportSurface::Initialize(
    gl::GLSurfaceFormat format) {
  DCHECK(!is_presentation_callback_enabled_ ||
         gl::GLSurfaceAdapter::SupportsPresentationCallback());
  // The surface is assumed to have already been initialized.
  delegate_->SetLatencyInfoCallback(
      base::Bind(&PassThroughImageTransportSurface::AddLatencyInfo,
                 base::Unretained(this)));
  return true;
}

void PassThroughImageTransportSurface::Destroy() {
  GLSurfaceAdapter::Destroy();
}

gfx::SwapResult PassThroughImageTransportSurface::SwapBuffers(
    const PresentationCallback& callback) {
  std::unique_ptr<std::vector<ui::LatencyInfo>> latency_info =
      StartSwapBuffers();
  gfx::SwapResult result = gl::GLSurfaceAdapter::SwapBuffers(
      base::Bind(&PassThroughImageTransportSurface::BufferPresented,
                 weak_ptr_factory_.GetWeakPtr(), swap_count_, callback));
  FinishSwapBuffers(swap_count_, std::move(latency_info), result);
  return result;
}

void PassThroughImageTransportSurface::SwapBuffersAsync(
    const SwapCompletionCallback& completion_callback,
    const PresentationCallback& presentation_callback) {
  std::unique_ptr<std::vector<ui::LatencyInfo>> latency_info =
      StartSwapBuffers();

  // We use WeakPtr here to avoid manual management of life time of an instance
  // of this class. Callback will not be called once the instance of this class
  // is destroyed. However, this also means that the callback can be run on
  // the calling thread only.
  gl::GLSurfaceAdapter::SwapBuffersAsync(
      base::Bind(&PassThroughImageTransportSurface::FinishSwapBuffersAsync,
                 weak_ptr_factory_.GetWeakPtr(), swap_count_,
                 base::Passed(&latency_info), completion_callback),
      base::Bind(&PassThroughImageTransportSurface::BufferPresented,
                 weak_ptr_factory_.GetWeakPtr(), swap_count_,
                 presentation_callback));
}

gfx::SwapResult PassThroughImageTransportSurface::SwapBuffersWithBounds(
    const std::vector<gfx::Rect>& rects,
    const PresentationCallback& callback) {
  std::unique_ptr<std::vector<ui::LatencyInfo>> latency_info =
      StartSwapBuffers();
  gfx::SwapResult result = gl::GLSurfaceAdapter::SwapBuffersWithBounds(
      rects, base::Bind(&PassThroughImageTransportSurface::BufferPresented,
                        weak_ptr_factory_.GetWeakPtr(), swap_count_, callback));
  FinishSwapBuffers(swap_count_, std::move(latency_info), result);
  return result;
}

gfx::SwapResult PassThroughImageTransportSurface::PostSubBuffer(
    int x,
    int y,
    int width,
    int height,
    const PresentationCallback& callback) {
  std::unique_ptr<std::vector<ui::LatencyInfo>> latency_info =
      StartSwapBuffers();
  gfx::SwapResult result = gl::GLSurfaceAdapter::PostSubBuffer(
      x, y, width, height,
      base::Bind(&PassThroughImageTransportSurface::BufferPresented,
                 weak_ptr_factory_.GetWeakPtr(), swap_count_, callback));
  FinishSwapBuffers(swap_count_, std::move(latency_info), result);
  return result;
}

void PassThroughImageTransportSurface::PostSubBufferAsync(
    int x,
    int y,
    int width,
    int height,
    const SwapCompletionCallback& completion_callback,
    const PresentationCallback& presentation_callback) {
  std::unique_ptr<std::vector<ui::LatencyInfo>> latency_info =
      StartSwapBuffers();
  gl::GLSurfaceAdapter::PostSubBufferAsync(
      x, y, width, height,
      base::Bind(&PassThroughImageTransportSurface::FinishSwapBuffersAsync,
                 weak_ptr_factory_.GetWeakPtr(), swap_count_,
                 base::Passed(&latency_info), completion_callback),
      base::Bind(&PassThroughImageTransportSurface::BufferPresented,
                 weak_ptr_factory_.GetWeakPtr(), swap_count_,
                 presentation_callback));
}

gfx::SwapResult PassThroughImageTransportSurface::CommitOverlayPlanes(
    const PresentationCallback& callback) {
  std::unique_ptr<std::vector<ui::LatencyInfo>> latency_info =
      StartSwapBuffers();
  gfx::SwapResult result = gl::GLSurfaceAdapter::CommitOverlayPlanes(
      base::Bind(&PassThroughImageTransportSurface::BufferPresented,
                 weak_ptr_factory_.GetWeakPtr(), swap_count_, callback));
  FinishSwapBuffers(swap_count_, std::move(latency_info), result);
  return result;
}

void PassThroughImageTransportSurface::CommitOverlayPlanesAsync(
    const SwapCompletionCallback& completion_callback,
    const PresentationCallback& presentation_callback) {
  std::unique_ptr<std::vector<ui::LatencyInfo>> latency_info =
      StartSwapBuffers();
  gl::GLSurfaceAdapter::CommitOverlayPlanesAsync(
      base::Bind(&PassThroughImageTransportSurface::FinishSwapBuffersAsync,
                 weak_ptr_factory_.GetWeakPtr(), swap_count_,
                 base::Passed(&latency_info), completion_callback),
      base::Bind(&PassThroughImageTransportSurface::BufferPresented,
                 weak_ptr_factory_.GetWeakPtr(), swap_count_,
                 presentation_callback));
}

PassThroughImageTransportSurface::~PassThroughImageTransportSurface() {
  if (delegate_) {
    delegate_->SetLatencyInfoCallback(
        base::Callback<void(const std::vector<ui::LatencyInfo>&)>());
  }
}

void PassThroughImageTransportSurface::AddLatencyInfo(
    const std::vector<ui::LatencyInfo>& latency_info) {
  latency_info_.insert(latency_info_.end(), latency_info.begin(),
                       latency_info.end());
}

void PassThroughImageTransportSurface::SendVSyncUpdateIfAvailable() {
  // When PresentationCallback is enabled, we will get VSYNC parameters from the
  // PresentationCallback, so the VSYNC update is not necessary anymore.
  if (is_presentation_callback_enabled_)
    return;

  gfx::VSyncProvider* vsync_provider = GetVSyncProvider();
  if (vsync_provider) {
    vsync_provider->GetVSyncParameters(base::Bind(
        &ImageTransportSurfaceDelegate::UpdateVSyncParameters, delegate_));
  }
}

void PassThroughImageTransportSurface::UpdateSwapInterval() {
  if (is_gpu_vsync_disabled_) {
    gl::GLContext::GetCurrent()->ForceSwapIntervalZero(true);
    return;
  }

  gl::GLContext::GetCurrent()->SetSwapInterval(1);

  if (multi_window_swap_interval_ == kMultiWindowSwapIntervalForceZero) {
    // This code is a simple way of enforcing that we only vsync if one surface
    // is swapping per frame. This provides single window cases a stable refresh
    // while allowing multi-window cases to not slow down due to multiple syncs
    // on a single thread. A better way to fix this problem would be to have
    // each surface present on its own thread.

    if (g_current_swap_generation_ == swap_generation_) {
      // No other surface has swapped since we swapped last time.
      if (g_num_swaps_in_current_swap_generation_ > 1)
        g_last_multi_window_swap_generation_ = g_current_swap_generation_;
      g_num_swaps_in_current_swap_generation_ = 0;
      g_current_swap_generation_++;
    }

    swap_generation_ = g_current_swap_generation_;
    g_num_swaps_in_current_swap_generation_++;

    bool should_override_vsync =
        (g_num_swaps_in_current_swap_generation_ > 1) ||
        (g_current_swap_generation_ - g_last_multi_window_swap_generation_ <
         kMultiWindowSwapEnableVSyncDelay);
    gl::GLContext::GetCurrent()->ForceSwapIntervalZero(should_override_vsync);
  }
}

std::unique_ptr<std::vector<ui::LatencyInfo>>
PassThroughImageTransportSurface::StartSwapBuffers() {
  // GetVsyncValues before SwapBuffers to work around Mali driver bug:
  // crbug.com/223558.
  SendVSyncUpdateIfAvailable();

  UpdateSwapInterval();

  ++swap_count_;

  base::TimeTicks swap_time = base::TimeTicks::Now();
  for (auto& latency : latency_info_) {
    latency.AddLatencyNumberWithTimestamp(
        ui::INPUT_EVENT_GPU_SWAP_BUFFER_COMPONENT, 0, 0, swap_time, 1);
  }

  std::unique_ptr<std::vector<ui::LatencyInfo>> latency_info(
      new std::vector<ui::LatencyInfo>());
  latency_info->swap(latency_info_);

  return latency_info;
}

void PassThroughImageTransportSurface::FinishSwapBuffers(
    uint32_t count,
    std::unique_ptr<std::vector<ui::LatencyInfo>> latency_info,
    gfx::SwapResult result) {
  base::TimeTicks swap_ack_time = base::TimeTicks::Now();
  bool has_browser_snapshot_request = false;
  for (auto& latency : *latency_info) {
    latency.AddLatencyNumberWithTimestamp(
        ui::INPUT_EVENT_LATENCY_TERMINATED_FRAME_SWAP_COMPONENT, 0, 0,
        swap_ack_time, 1);
    has_browser_snapshot_request |= latency.FindLatency(
        ui::BROWSER_SNAPSHOT_FRAME_NUMBER_COMPONENT, nullptr);
  }
  if (has_browser_snapshot_request)
    WaitForSnapshotRendering();

  if (delegate_) {
    SwapBuffersCompleteParams params;
    params.latency_info = std::move(*latency_info);
    params.result = result;
    params.count = count;
    delegate_->DidSwapBuffersComplete(std::move(params));
  }
}

void PassThroughImageTransportSurface::FinishSwapBuffersAsync(
    uint32_t count,
    std::unique_ptr<std::vector<ui::LatencyInfo>> latency_info,
    GLSurface::SwapCompletionCallback callback,
    gfx::SwapResult result) {
  FinishSwapBuffers(count, std::move(latency_info), result);
  callback.Run(result);
}

void PassThroughImageTransportSurface::BufferPresented(
    uint32_t count,
    const GLSurface::PresentationCallback& callback,
    base::TimeTicks timestamp,
    base::TimeDelta refresh,
    uint32_t flags) {
  if (!is_presentation_callback_enabled_)
    return;
  callback.Run(timestamp, refresh, flags);
  if (delegate_)
    delegate_->BufferPresented(count, timestamp, refresh, flags);
}

}  // namespace gpu
