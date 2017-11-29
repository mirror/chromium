// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/viz/service/display_embedder/software_output_surface.h"

#include <utility>

#include "base/bind.h"
#include "base/location.h"
#include "base/memory/ref_counted.h"
#include "base/threading/sequenced_task_runner_handle.h"
#include "base/time/time.h"
#include "build/build_config.h"
#include "components/viz/service/display/output_surface_client.h"
#include "components/viz/service/display/output_surface_frame.h"
#include "components/viz/service/display/software_output_device.h"
#include "ui/gfx/vsync_provider.h"
#include "ui/latency/latency_info.h"

namespace viz {

SoftwareOutputSurface::SoftwareOutputSurface(
    std::unique_ptr<SoftwareOutputDevice> software_device,
    scoped_refptr<base::SequencedTaskRunner> task_runner)
    : OutputSurface(std::move(software_device)),
      task_runner_(std::move(task_runner)),
      weak_factory_(this) {}

SoftwareOutputSurface::~SoftwareOutputSurface() {}

void SoftwareOutputSurface::BindToClient(OutputSurfaceClient* client) {
  DCHECK(client);
  DCHECK(!client_);
  client_ = client;
}

void SoftwareOutputSurface::EnsureBackbuffer() {
  software_device()->EnsureBackbuffer();
}

void SoftwareOutputSurface::DiscardBackbuffer() {
  software_device()->DiscardBackbuffer();
}

void SoftwareOutputSurface::BindFramebuffer() {
  // Not used for software surfaces.
  NOTREACHED();
}

void SoftwareOutputSurface::SetDrawRectangle(const gfx::Rect& draw_rectangle) {
  NOTREACHED();
}

void SoftwareOutputSurface::Reshape(const gfx::Size& size,
                                    float device_scale_factor,
                                    const gfx::ColorSpace& color_space,
                                    bool has_alpha,
                                    bool use_stencil) {
  software_device()->Resize(size, device_scale_factor);
}

void SoftwareOutputSurface::SwapBuffers(OutputSurfaceFrame frame) {
  DCHECK(client_);
  base::TimeTicks swap_time = base::TimeTicks::Now();
  for (auto& latency : frame.latency_info) {
    latency.AddLatencyNumberWithTimestamp(
        ui::INPUT_EVENT_GPU_SWAP_BUFFER_COMPONENT, 0, 0, swap_time, 1);
    latency.AddLatencyNumberWithTimestamp(
        ui::INPUT_EVENT_LATENCY_TERMINATED_FRAME_SWAP_COMPONENT, 0, 0,
        swap_time, 1);
  }
  // TODO(danakj): Send frame.latency_info somewhere like
  // RenderWidgetHostImpl::OnGpuSwapBuffersCompleted.

  // TODO(danakj): Update vsync params.
  // gfx::VSyncProvider* vsync_provider = software_device()->GetVSyncProvider();
  // if (vsync_provider)
  //  vsync_provider->GetVSyncParameters(update_vsync_parameters_callback_);

  task_runner_->PostTask(
      FROM_HERE, base::BindOnce(&SoftwareOutputSurface::SwapBuffersCallback,
                                weak_factory_.GetWeakPtr()));
}

void SoftwareOutputSurface::SwapBuffersCallback() {
  client_->DidReceiveSwapBuffersAck();
}

bool SoftwareOutputSurface::IsDisplayedAsOverlayPlane() const {
  return false;
}

OverlayCandidateValidator* SoftwareOutputSurface::GetOverlayCandidateValidator()
    const {
  // No overlay support in software compositing.
  return nullptr;
}

unsigned SoftwareOutputSurface::GetOverlayTextureId() const {
  return 0;
}

gfx::BufferFormat SoftwareOutputSurface::GetOverlayBufferFormat() const {
  return gfx::BufferFormat::RGBX_8888;
}

bool SoftwareOutputSurface::HasExternalStencilTest() const {
  return false;
}

void SoftwareOutputSurface::ApplyExternalStencil() {}

bool SoftwareOutputSurface::SurfaceIsSuspendForRecycle() const {
  return false;
}

uint32_t SoftwareOutputSurface::GetFramebufferCopyTextureFormat() {
  // Not used for software surfaces.
  NOTREACHED();
  return 0;
}

}  // namespace viz
