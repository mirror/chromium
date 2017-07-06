// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_VIZ_SERVICE_FRAME_SINKS_DIRECT_LAYER_TREE_FRAME_SINK_H_
#define COMPONENTS_VIZ_SERVICE_FRAME_SINKS_DIRECT_LAYER_TREE_FRAME_SINK_H_

#include "base/macros.h"
#include "base/threading/thread_checker.h"
#include "cc/output/layer_tree_frame_sink.h"
#include "cc/scheduler/begin_frame_source.h"
#include "cc/surfaces/compositor_frame_sink_support.h"
#include "cc/surfaces/compositor_frame_sink_support_client.h"
#include "cc/surfaces/display_client.h"
#include "cc/surfaces/local_surface_id_allocator.h"
//#include "cc/surfaces/surfaces_export.h"
#include "components/viz/service/viz_service_export.h"

namespace cc {
class Display;
class LocalSurfaceIdAllocator;
class SurfaceManager;
}  // namespace cc

namespace viz {

// This class submits compositor frames to an in-process Display, with the
// client's frame being the root surface of the Display.
class VIZ_SERVICE_EXPORT DirectLayerTreeFrameSink
    : public cc::LayerTreeFrameSink,
      public NON_EXPORTED_BASE(cc::CompositorFrameSinkSupportClient),
      public cc::ExternalBeginFrameSourceClient,
      public NON_EXPORTED_BASE(cc::DisplayClient) {
 public:
  // The underlying Display, SurfaceManager, and LocalSurfaceIdAllocator must
  // outlive this class.
  DirectLayerTreeFrameSink(
      const cc::FrameSinkId& frame_sink_id,
      cc::SurfaceManager* surface_manager,
      cc::Display* display,
      scoped_refptr<cc::ContextProvider> context_provider,
      scoped_refptr<cc::ContextProvider> worker_context_provider,
      gpu::GpuMemoryBufferManager* gpu_memory_buffer_manager,
      cc::SharedBitmapManager* shared_bitmap_manager);
  DirectLayerTreeFrameSink(
      const cc::FrameSinkId& frame_sink_id,
      cc::SurfaceManager* surface_manager,
      cc::Display* display,
      scoped_refptr<cc::VulkanContextProvider> vulkan_context_provider);
  ~DirectLayerTreeFrameSink() override;

  // LayerTreeFrameSink implementation.
  bool BindToClient(cc::LayerTreeFrameSinkClient* client) override;
  void DetachFromClient() override;
  void SubmitCompositorFrame(cc::CompositorFrame frame) override;
  void DidNotProduceFrame(const cc::BeginFrameAck& ack) override;

  // DisplayClient implementation.
  void DisplayOutputSurfaceLost() override;
  void DisplayWillDrawAndSwap(bool will_draw_and_swap,
                              const cc::RenderPassList& render_passes) override;
  void DisplayDidDrawAndSwap() override;

 protected:
  std::unique_ptr<cc::CompositorFrameSinkSupport>
      support_;  // protected for test.

 private:
  // CompositorFrameSinkSupportClient implementation:
  void DidReceiveCompositorFrameAck(
      const std::vector<cc::ReturnedResource>& resources) override;
  void OnBeginFrame(const cc::BeginFrameArgs& args) override;
  void ReclaimResources(
      const std::vector<cc::ReturnedResource>& resources) override;
  void WillDrawSurface(const cc::LocalSurfaceId& local_surface_id,
                       const gfx::Rect& damage_rect) override;

  // ExternalBeginFrameSourceClient implementation:
  void OnNeedsBeginFrames(bool needs_begin_frame) override;

  // This class is only meant to be used on a single thread.
  base::ThreadChecker thread_checker_;

  const cc::FrameSinkId frame_sink_id_;
  cc::LocalSurfaceId local_surface_id_;
  cc::SurfaceManager* surface_manager_;
  cc::LocalSurfaceIdAllocator local_surface_id_allocator_;
  cc::Display* display_;
  gfx::Size last_swap_frame_size_;
  float device_scale_factor_ = 1.f;
  bool is_lost_ = false;
  std::unique_ptr<cc::ExternalBeginFrameSource> begin_frame_source_;

  DISALLOW_COPY_AND_ASSIGN(DirectLayerTreeFrameSink);
};

}  // namespace viz

#endif  // COMPONENTS_VIZ_SERVICE_FRAME_SINKS_DIRECT_LAYER_TREE_FRAME_SINK_H_
