// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_VIZ_TEST_FAKE_COMPOSITOR_FRAME_SINK_CLIENT_H_
#define COMPONENTS_VIZ_TEST_FAKE_COMPOSITOR_FRAME_SINK_CLIENT_H_

#include "components/viz/common/surfaces/local_surface_id.h"
#include "services/viz/public/interfaces/compositing/compositor_frame_sink.mojom.h"
#include "ui/gfx/geometry/rect.h"

namespace viz {

class FakeCompositorFrameSinkClient : public mojom::CompositorFrameSinkClient {
 public:
  FakeCompositorFrameSinkClient();
  ~FakeCompositorFrameSinkClient() override;

  // mojom::CompositorFrameSinkClient implementation.
  void DidReceiveCompositorFrameAck(
      const std::vector<ReturnedResource>& resources) override;
  void OnBeginFrame(const BeginFrameArgs& args) override;
  void ReclaimResources(
      const std::vector<ReturnedResource>& resources) override;
  void OnBeginFramePausedChanged(bool paused) override;

  void WillDrawSurface(const LocalSurfaceId& local_surface_id,
                       const gfx::Rect& damage_rect);

  const gfx::Rect& last_damage_rect() const { return last_damage_rect_; }
  const LocalSurfaceId& last_local_surface_id() const {
    return last_local_surface_id_;
  }
  const std::vector<ReturnedResource>& returned_resources() const {
    return returned_resources_;
  }

 private:
  gfx::Rect last_damage_rect_;
  LocalSurfaceId last_local_surface_id_;
  std::vector<ReturnedResource> returned_resources_;

  DISALLOW_COPY_AND_ASSIGN(FakeCompositorFrameSinkClient);
};

}  // namespace viz

#endif  // COMPONENTS_VIZ_TEST_FAKE_COMPOSITOR_FRAME_SINK_CLIENT_H_
