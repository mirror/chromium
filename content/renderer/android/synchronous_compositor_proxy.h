// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_RENDERER_ANDROID_SYNCHRONOUS_COMPOSITOR_PROXY_H_
#define CONTENT_RENDERER_ANDROID_SYNCHRONOUS_COMPOSITOR_PROXY_H_

#include <stddef.h>
#include <stdint.h>

#include "base/callback.h"
#include "base/macros.h"
#include "base/optional.h"
#include "content/common/input/synchronous_compositor.mojom.h"
#include "content/public/common/input_event_ack_state.h"
#include "content/renderer/android/synchronous_layer_tree_frame_sink.h"
#include "mojo/public/cpp/bindings/associated_binding.h"
#include "ui/events/blink/synchronous_input_handler_proxy.h"
#include "ui/gfx/geometry/scroll_offset.h"
#include "ui/gfx/geometry/size_f.h"

namespace viz {
namespace cc {
class CompositorFrame;
}  // namespace cc
}  // namespace viz

namespace content {

class SynchronousLayerTreeFrameSink;
struct SyncCompositorCommonRendererParams;
struct SyncCompositorDemandDrawHwParams;
struct SyncCompositorDemandDrawSwParams;
struct SyncCompositorSetSharedMemoryParams;

class SynchronousCompositorProxy : public ui::SynchronousInputHandler,
                                   public SynchronousLayerTreeFrameSinkClient,
                                   public mojom::SynchronousCompositor {
 public:
  SynchronousCompositorProxy(
      ui::SynchronousInputHandlerProxy* input_handler_proxy);
  ~SynchronousCompositorProxy() override;

  void Init();

  // ui::SynchronousInputHandler overrides.
  void SetNeedsSynchronousAnimateInput() override;
  void UpdateRootLayerState(const gfx::ScrollOffset& total_scroll_offset,
                            const gfx::ScrollOffset& max_scroll_offset,
                            const gfx::SizeF& scrollable_size,
                            float page_scale_factor,
                            float min_page_scale_factor,
                            float max_page_scale_factor) override;

  // SynchronousLayerTreeFrameSinkClient overrides.
  void DidActivatePendingTree() override;
  void Invalidate() override;
  void SubmitCompositorFrame(uint32_t layer_tree_frame_sink_id,
                             viz::CompositorFrame frame) override;
  void SinkDestroyed() override;

  void SetLayerTreeFrameSink(
      SynchronousLayerTreeFrameSink* layer_tree_frame_sink);
  void PopulateCommonParams(SyncCompositorCommonRendererParams* params);

  // mojom::SynchronousCompositor overrides.
  void SynchronizeState() override;
  void ComputeScroll(base::TimeTicks animation_time) override;
  void DemandDrawHwAsync(
      const SyncCompositorDemandDrawHwParams& draw_params) override;
  void DemandDrawHw(const SyncCompositorDemandDrawHwParams& params,
                    DemandDrawHwCallback callback) override;
  void SetSharedMemory(const SyncCompositorSetSharedMemoryParams& params,
                       SetSharedMemoryCallback callback) override;
  void DemandDrawSw(const SyncCompositorDemandDrawSwParams& params,
                    DemandDrawSwCallback callback) override;
  void ZeroSharedMemory() override;
  void ZoomBy(float zoom_delta,
              const gfx::Point& anchor,
              ZoomByCallback) override;
  void SetMemoryPolicy(uint32_t bytes_limit) override;
  void ReclaimResources(
      uint32_t layer_tree_frame_sink_id,
      const std::vector<viz::ReturnedResource>& resources) override;
  void SetScroll(const gfx::ScrollOffset& total_scroll_offset) override;

  void BindChannel(
      mojom::SynchronousCompositorControlHostPtr control_host,
      mojom::SynchronousCompositorHostAssociatedPtrInfo host,
      mojom::SynchronousCompositorAssociatedRequest compositor_request);

 protected:
  virtual void SendAsyncRendererStateIfNeeded();
  virtual void LayerTreeFrameSinkCreated();
  void DemandDrawHwAsyncReply(
      const content::SyncCompositorCommonRendererParams&,
      uint32_t,
      base::Optional<viz::CompositorFrame>);

  struct SharedMemoryWithSize;

  ui::SynchronousInputHandlerProxy* const input_handler_proxy_;
  const bool use_in_process_zero_copy_software_draw_;
  SynchronousLayerTreeFrameSink* layer_tree_frame_sink_;
  DemandDrawHwCallback hardware_draw_reply_;
  DemandDrawSwCallback software_draw_reply_;
  ZoomByCallback zoom_by_reply_;

  // From browser.
  std::unique_ptr<SharedMemoryWithSize> software_draw_shm_;

  // To browser.
  uint32_t version_;
  gfx::ScrollOffset total_scroll_offset_;  // Modified by both.
  gfx::ScrollOffset max_scroll_offset_;
  gfx::SizeF scrollable_size_;
  float page_scale_factor_;
  float min_page_scale_factor_;
  float max_page_scale_factor_;
  bool need_animate_scroll_;
  bool needs_layer_frame_sink_create_ = false;
  uint32_t need_invalidate_count_;
  uint32_t did_activate_pending_tree_count_;

 private:
  mojom::SynchronousCompositorControlHostPtr control_host_;
  mojom::SynchronousCompositorHostAssociatedPtr host_;
  mojo::AssociatedBinding<mojom::SynchronousCompositor> binding_;

  DISALLOW_COPY_AND_ASSIGN(SynchronousCompositorProxy);
};

}  // namespace content

#endif  // CONTENT_RENDERER_ANDROID_SYNCHRONOUS_COMPOSITOR_PROXY_H_
