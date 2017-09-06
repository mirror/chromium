// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/viz/service/frame_sinks/root_compositor_frame_sink_impl.h"

#include <utility>

#include "components/viz/common/frame_sinks/begin_frame_source.h"
#include "components/viz/service/display/display.h"
#include "components/viz/service/frame_sinks/frame_sink_manager_impl.h"

namespace viz {

RootCompositorFrameSinkImpl::RootCompositorFrameSinkImpl(
    FrameSinkManagerImpl* frame_sink_manager,
    const FrameSinkId& frame_sink_id,
    std::unique_ptr<Display> display,
    std::unique_ptr<BeginFrameSource> begin_frame_source,
    mojom::CompositorFrameSinkAssociatedRequest request,
    mojom::CompositorFrameSinkClientPtr client,
    mojom::DisplayPrivateAssociatedRequest display_private_request)
    : CompositorFrameSinkSupport(client.get(),
                                 frame_sink_id,
                                 true /* is_root */,
                                 true /* needs_sync_points */),
      display_begin_frame_source_(std::move(begin_frame_source)),
      display_(std::move(display)),
      compositor_frame_sink_client_(std::move(client)),
      compositor_frame_sink_binding_(this, std::move(request)),
      display_private_binding_(this, std::move(display_private_request)),
      hit_test_aggregator_(this) {
  DCHECK(display_begin_frame_source_);
  DCHECK(display_);

  Init(frame_sink_manager);
  compositor_frame_sink_binding_.set_connection_error_handler(
      base::Bind(&RootCompositorFrameSinkImpl::OnClientConnectionLost,
                 base::Unretained(this)));
  frame_sink_manager->RegisterBeginFrameSource(
      display_begin_frame_source_.get(), frame_sink_id);
  display_->Initialize(this, frame_sink_manager->surface_manager());
}

RootCompositorFrameSinkImpl::~RootCompositorFrameSinkImpl() {
  frame_sink_manager()->UnregisterBeginFrameSource(
      display_begin_frame_source_.get());
  // Reset before destroying |compositor_frame_sink_client_|.
  ResetClientForDestructor();
}

void RootCompositorFrameSinkImpl::SetDisplayVisible(bool visible) {
  display_->SetVisible(visible);
}

void RootCompositorFrameSinkImpl::ResizeDisplay(const gfx::Size& size) {
  display_->Resize(size);
}

void RootCompositorFrameSinkImpl::SetDisplayColorSpace(
    const gfx::ColorSpace& color_space) {
  display_->SetColorSpace(color_space, color_space);
}

void RootCompositorFrameSinkImpl::SetOutputIsSecure(bool secure) {
  display_->SetOutputIsSecure(secure);
}

void RootCompositorFrameSinkImpl::SetLocalSurfaceId(
    const LocalSurfaceId& local_surface_id,
    float scale_factor) {
  display_->SetLocalSurfaceId(local_surface_id, scale_factor);
}

void RootCompositorFrameSinkImpl::SubmitCompositorFrame(
    const LocalSurfaceId& local_surface_id,
    cc::CompositorFrame frame,
    mojom::HitTestRegionListPtr hit_test_region_list,
    uint64_t submit_time) {
  if (!CompositorFrameSinkSupport::SubmitCompositorFrame(
          local_surface_id, std::move(frame),
          std::move(hit_test_region_list))) {
    DLOG(ERROR) << "SubmitCompositorFrame failed for " << local_surface_id;
    compositor_frame_sink_binding_.Close();
    OnClientConnectionLost();
  }
}

void RootCompositorFrameSinkImpl::OnAggregatedHitTestRegionListUpdated(
    mojo::ScopedSharedBufferHandle active_handle,
    uint32_t active_handle_size,
    mojo::ScopedSharedBufferHandle idle_handle,
    uint32_t idle_handle_size) {
  frame_sink_manager()->OnAggregatedHitTestRegionListUpdated(
      frame_sink_id(), std::move(active_handle), active_handle_size,
      std::move(idle_handle), idle_handle_size);
}

void RootCompositorFrameSinkImpl::SwitchActiveAggregatedHitTestRegionList(
    uint8_t active_handle_index) {
  frame_sink_manager()->SwitchActiveAggregatedHitTestRegionList(
      frame_sink_id(), active_handle_index);
}

void RootCompositorFrameSinkImpl::OnClientConnectionLost() {
  frame_sink_manager()->OnClientConnectionLost(frame_sink_id());
}

void RootCompositorFrameSinkImpl::DisplayOutputSurfaceLost() {
  // TODO(staraz): Implement this. Client should hear about context/output
  // surface lost.
}

void RootCompositorFrameSinkImpl::DisplayWillDrawAndSwap(
    bool will_draw_and_swap,
    const cc::RenderPassList& render_pass) {
  hit_test_aggregator_.PostTaskAggregate(display_->CurrentSurfaceId());
}

void RootCompositorFrameSinkImpl::DisplayDidDrawAndSwap() {}


}  // namespace viz
