// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/aura/local/compositor_frame_sink_mojo.h"

#include "cc/output/compositor_frame_sink_client.h"
#include "cc/surfaces/compositor_frame_sink_support.h"
#include "ui/aura/client/cursor_client.h"
#include "ui/aura/env.h"
#include "ui/aura/window.h"
#include "ui/aura/window_delegate.h"
#include "ui/display/display.h"
#include "ui/display/screen.h"

namespace aura {

CompositorFrameSinkMojo::CompositorFrameSinkMojo(
    const cc::FrameSinkId& frame_sink_id,
    cc::SurfaceManager* surface_manager,
    cc::mojom::MojoCompositorFrameSinkClientPtr client)
    : frame_sink_id_(frame_sink_id),
      surface_manager_(surface_manager),
      client_(std::move(client)) {
  support_ = cc::CompositorFrameSinkSupport::Create(
      this, surface_manager_, frame_sink_id_, false /* is_root */,
      true /* handles_frame_sink_id_invalidation */,
      true /* needs_sync_points */);
}

CompositorFrameSinkMojo::~CompositorFrameSinkMojo() {}

void CompositorFrameSinkMojo::SetSurfaceChangedCallback(
    const SurfaceChangedCallback& callback) {
  DCHECK(!surface_changed_callback_);
  surface_changed_callback_ = callback;
}

void CompositorFrameSinkMojo::SetNeedsBeginFrame(bool needs_begin_frame) {
  support_->SetNeedsBeginFrame(needs_begin_frame);
}

void CompositorFrameSinkMojo::SubmitCompositorFrame(
    const cc::LocalSurfaceId& local_surface_id,
    cc::CompositorFrame frame) {
  if (local_surface_id_ != local_surface_id) {
    local_surface_id_ = local_surface_id;
    const auto& frame_size = frame.render_pass_list.back()->output_rect.size();
    surface_changed_callback_.Run(
        cc::SurfaceId(frame_sink_id_, local_surface_id_), frame_size);
  }
  bool result =
      support_->SubmitCompositorFrame(local_surface_id_, std::move(frame));
  DCHECK(result);
}

void CompositorFrameSinkMojo::DidNotProduceFrame(const cc::BeginFrameAck& ack) {
  DCHECK(!ack.has_damage);
  DCHECK_LE(cc::BeginFrameArgs::kStartingFrameNumber, ack.sequence_number);
  support_->DidNotProduceFrame(ack);
}

void CompositorFrameSinkMojo::DidReceiveCompositorFrameAck(
    const cc::ReturnedResourceArray& resources) {
  client_->DidReceiveCompositorFrameAck(resources);
}

void CompositorFrameSinkMojo::OnBeginFrame(const cc::BeginFrameArgs& args) {
  client_->OnBeginFrame(args);
}

void CompositorFrameSinkMojo::ReclaimResources(
    const cc::ReturnedResourceArray& resources) {
  client_->ReclaimResources(resources);
}

}  // namespace aura
