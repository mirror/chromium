// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/viz/test/fake_compositor_frame_sink_client.h"

#include "components/viz/service/surfaces/surface_manager.h"

namespace viz {

FakeCompositorFrameSinkClient::FakeCompositorFrameSinkClient() = default;
FakeCompositorFrameSinkClient::~FakeCompositorFrameSinkClient() = default;

void FakeCompositorFrameSinkClient::DidReceiveCompositorFrameAck(
    const std::vector<ReturnedResource>& resources) {
  returned_resources_ = resources;
}

void FakeCompositorFrameSinkClient::OnBeginFrame(const BeginFrameArgs& args) {}

void FakeCompositorFrameSinkClient::ReclaimResources(
    const std::vector<ReturnedResource>& resources) {
  returned_resources_ = resources;
}

void FakeCompositorFrameSinkClient::OnBeginFramePausedChanged(bool paused) {}

void FakeCompositorFrameSinkClient::WillDrawSurface(
    const LocalSurfaceId& local_surface_id,
    const gfx::Rect& damage_rect) {
  last_local_surface_id_ = local_surface_id;
  last_damage_rect_ = damage_rect;
}

};  // namespace viz
