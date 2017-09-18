// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/exo/layer_tree_frame_sink_holder.h"

#include "base/threading/thread_task_runner_handle.h"
#include "cc/output/compositor_frame.h"
#include "cc/output/layer_tree_frame_sink.h"
#include "components/exo/surface_tree_host.h"
#include "components/viz/common/quads/render_pass.h"
#include "components/viz/common/resources/returned_resource.h"

namespace exo {

////////////////////////////////////////////////////////////////////////////////
// LayerTreeFrameSinkHolder, public:

LayerTreeFrameSinkHolder::LayerTreeFrameSinkHolder(
    SurfaceTreeHost* surface_tree_host,
    std::unique_ptr<cc::LayerTreeFrameSink> frame_sink)
    : surface_tree_host_(surface_tree_host),
      frame_sink_(std::move(frame_sink)),
      weak_factory_(this) {
  frame_sink_->BindToClient(this);
}

LayerTreeFrameSinkHolder::~LayerTreeFrameSinkHolder() {
  frame_sink_->DetachFromClient();
  DCHECK(release_callbacks_.empty());
}

void LayerTreeFrameSinkHolder::Delete() {
  DCHECK(!is_deleted_);
  is_deleted_ = true;
  weak_factory_.InvalidateWeakPtrs();
  if (!release_callbacks_.empty()) {
    cc::CompositorFrame frame;
    frame.metadata.begin_frame_ack =
        viz::BeginFrameAck::CreateManualAckWithDamage();
    frame.render_pass_list.push_back(viz::RenderPass::Create());
    frame_sink_->SubmitCompositorFrame(std::move(frame));
  } else {
    delete this;
  }
}

bool LayerTreeFrameSinkHolder::HasReleaseCallbackForResource(
    viz::ResourceId id) {
  DCHECK(!is_deleted_);
  return release_callbacks_.find(id) != release_callbacks_.end();
}

void LayerTreeFrameSinkHolder::SetResourceReleaseCallback(
    viz::ResourceId id,
    const viz::ReleaseCallback& callback) {
  DCHECK(!is_deleted_);
  DCHECK(!callback.is_null());
  release_callbacks_[id] = callback;
}

int LayerTreeFrameSinkHolder::AllocateResourceId() {
  DCHECK(!is_deleted_);
  return next_resource_id_++;
}

base::WeakPtr<LayerTreeFrameSinkHolder> LayerTreeFrameSinkHolder::GetWeakPtr() {
  DCHECK(!is_deleted_);
  return weak_factory_.GetWeakPtr();
}

////////////////////////////////////////////////////////////////////////////////
// cc::LayerTreeFrameSinkClient overrides:

void LayerTreeFrameSinkHolder::SetBeginFrameSource(
    viz::BeginFrameSource* source) {
  if (surface_tree_host_)
    surface_tree_host_->SetBeginFrameSource(source);
}

void LayerTreeFrameSinkHolder::ReclaimResources(
    const std::vector<viz::ReturnedResource>& resources) {
  for (auto& resource : resources) {
    auto it = release_callbacks_.find(resource.id);
    DCHECK(it != release_callbacks_.end());
    if (it != release_callbacks_.end()) {
      it->second.Run(resource.sync_token, resource.lost);
      release_callbacks_.erase(it);
    }
  }
  if (is_deleted_ && release_callbacks_.empty())
    base::ThreadTaskRunnerHandle::Get()->DeleteSoon(FROM_HERE, this);
}

void LayerTreeFrameSinkHolder::DidReceiveCompositorFrameAck() {
  surface_tree_host_->DidReceiveCompositorFrameAck();
}

}  // namespace exo
