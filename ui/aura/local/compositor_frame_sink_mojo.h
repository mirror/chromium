// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_AURA_LOCAL_COMPOSITOR_FRAME_SINK_MOJO_H_
#define UI_AURA_LOCAL_COMPOSITOR_FRAME_SINK_MOJO_H_

#include "base/callback.h"
#include "base/macros.h"
#include "cc/ipc/mojo_compositor_frame_sink.mojom.h"
#include "cc/scheduler/begin_frame_source.h"
#include "cc/surfaces/compositor_frame_sink_support_client.h"
#include "cc/surfaces/frame_sink_id.h"
#include "cc/surfaces/local_surface_id_allocator.h"
#include "ui/aura/window_port.h"
#include "ui/base/property_data.h"

namespace cc {
class CompositorFrameSinkSupport;
class SurfaceManager;
}

namespace aura {

// cc::CompositorFrameSink implementation for classic aura, e.g. not mus.
// aura::Window::CreateCompositorFramSink creates this class for a given
// aura::Window, and then the sink can be used for submitting frames to the
// aura::Window's ui::Layer.
class CompositorFrameSinkMojo : public cc::mojom::MojoCompositorFrameSink,
                                public cc::CompositorFrameSinkSupportClient {
 public:
  CompositorFrameSinkMojo(const cc::FrameSinkId& frame_sink_id,
                          cc::SurfaceManager* surface_manager,
                          cc::mojom::MojoCompositorFrameSinkClientPtr client);
  ~CompositorFrameSinkMojo() override;

  using SurfaceChangedCallback =
      base::Callback<void(const cc::SurfaceId&, const gfx::Size&)>;
  // Set a callback which will be called when the surface is changed.
  void SetSurfaceChangedCallback(const SurfaceChangedCallback& callback);

  // cc::mojom::MojoCompositorFrameSink:
  void SetNeedsBeginFrame(bool needs_begin_frame) override;
  void SubmitCompositorFrame(const cc::LocalSurfaceId& local_surface_id,
                             cc::CompositorFrame frame) override;
  void DidNotProduceFrame(const cc::BeginFrameAck& ack) override;

  // cc::CompositorFrameSinkSupportClient:
  void DidReceiveCompositorFrameAck(
      const cc::ReturnedResourceArray& resources) override;
  void OnBeginFrame(const cc::BeginFrameArgs& args) override;
  void ReclaimResources(const cc::ReturnedResourceArray& resources) override;
  void WillDrawSurface(const cc::LocalSurfaceId& local_surface_id,
                       const gfx::Rect& damage_rect) override {}

 private:
  const cc::FrameSinkId frame_sink_id_;
  cc::SurfaceManager* const surface_manager_;
  std::unique_ptr<cc::CompositorFrameSinkSupport> support_;
  cc::LocalSurfaceId local_surface_id_;
  SurfaceChangedCallback surface_changed_callback_;
  cc::mojom::MojoCompositorFrameSinkClientPtr client_;

  DISALLOW_COPY_AND_ASSIGN(CompositorFrameSinkMojo);
};

}  // namespace aura

#endif  // UI_AURA_LOCAL_COMPOSITOR_FRAME_SINK_MOJO_H_
