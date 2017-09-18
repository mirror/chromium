// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_EXO_LAYER_TREE_FRAME_SINK_HOLDER_H_
#define COMPONENTS_EXO_LAYER_TREE_FRAME_SINK_HOLDER_H_

#include <memory>

#include "base/containers/flat_map.h"
#include "cc/output/layer_tree_frame_sink_client.h"
#include "components/viz/common/quads/release_callback.h"

namespace base {
template <class T>
class DeleteHelper;
}

namespace cc {
class LayerTreeFrameSink;
}

namespace exo {
class SurfaceTreeHost;

// This class talks to CompositorFrameSink and keeps track of references to
// the contents of Buffers.
class LayerTreeFrameSinkHolder : public cc::LayerTreeFrameSinkClient {
 public:
  LayerTreeFrameSinkHolder(SurfaceTreeHost* surface_tree_host,
                           std::unique_ptr<cc::LayerTreeFrameSink> frame_sink);
  void Delete();

  bool HasReleaseCallbackForResource(viz::ResourceId id);
  void SetResourceReleaseCallback(viz::ResourceId id,
                                  const viz::ReleaseCallback& callback);
  int AllocateResourceId();

  // Delete the |LayerTreeFrameSinkHolder|, and it should not be used when this
  // function is called.
  base::WeakPtr<LayerTreeFrameSinkHolder> GetWeakPtr();

  cc::LayerTreeFrameSink* frame_sink() { return frame_sink_.get(); }

  // Overridden from cc::LayerTreeFrameSinkClient:
  void SetBeginFrameSource(viz::BeginFrameSource* source) override;
  void ReclaimResources(
      const std::vector<viz::ReturnedResource>& resources) override;
  void SetTreeActivationCallback(const base::Closure& callback) override {}
  void DidReceiveCompositorFrameAck() override;
  void DidLoseLayerTreeFrameSink() override {}
  void OnDraw(const gfx::Transform& transform,
              const gfx::Rect& viewport,
              bool resourceless_software_draw) override {}
  void SetMemoryPolicy(const cc::ManagedMemoryPolicy& policy) override {}
  void SetExternalTilePriorityConstraints(
      const gfx::Rect& viewport_rect,
      const gfx::Transform& transform) override {}

 private:
  friend base::DeleteHelper<LayerTreeFrameSinkHolder>;

  ~LayerTreeFrameSinkHolder() override;

  // A collection of callbacks used to release resources.
  using ResourceReleaseCallbackMap =
      base::flat_map<viz::ResourceId, viz::ReleaseCallback>;
  ResourceReleaseCallbackMap release_callbacks_;

  SurfaceTreeHost* surface_tree_host_;
  std::unique_ptr<cc::LayerTreeFrameSink> frame_sink_;

  // The next resource id the buffer is attached to.
  int next_resource_id_ = 1;

  bool is_deleted_ = false;

  base::WeakPtrFactory<LayerTreeFrameSinkHolder> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(LayerTreeFrameSinkHolder);
};

}  // namespace exo

namespace std {
// Provide the delector for std::unique_ptr<exo::LayerTreeFrameSinkHolder>.
template <>
struct default_delete<exo::LayerTreeFrameSinkHolder> {
  void operator()(exo::LayerTreeFrameSinkHolder* holder) { holder->Delete(); }
};
}  // namespace std

#endif  // COMPONENTS_EXO_LAYER_TREE_FRAME_SINK_HOLDER_H_
