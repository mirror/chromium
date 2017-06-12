// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/viz/host/frame_sink_manager_host.h"

#include <utility>

#include "base/command_line.h"
#include "cc/base/switches.h"
#include "cc/surfaces/surface_info.h"
#include "cc/surfaces/surface_manager.h"

namespace viz {
namespace {

bool UseSurfaceReferences() {
  return base::CommandLine::ForCurrentProcess()->HasSwitch(
      cc::switches::kEnableSurfaceReferences);
}

}  // namespace

FrameSinkManagerHost::FrameSinkManagerHost()
    : binding_(this), frame_sink_manager_(UseSurfaceReferences(), nullptr) {}

FrameSinkManagerHost::~FrameSinkManagerHost() {}

cc::SurfaceManager* FrameSinkManagerHost::surface_manager() {
  return frame_sink_manager_.surface_manager();
}

void FrameSinkManagerHost::ConnectToFrameSinkManager() {
  DCHECK(!frame_sink_manager_ptr_.is_bound());
  cc::mojom::FrameSinkManagerClientPtr client;
  binding_.Bind(mojo::MakeRequest(&client));
  frame_sink_manager_.Connect(mojo::MakeRequest(&frame_sink_manager_ptr_),
                              std::move(client));
}

void FrameSinkManagerHost::AddObserver(FrameSinkObserver* observer) {
  observers_.AddObserver(observer);
}

void FrameSinkManagerHost::RemoveObserver(FrameSinkObserver* observer) {
  observers_.RemoveObserver(observer);
}

void FrameSinkManagerHost::CreateCompositorFrameSink(
    const cc::FrameSinkId& frame_sink_id,
    cc::mojom::MojoCompositorFrameSinkRequest request,
    cc::mojom::MojoCompositorFrameSinkPrivateRequest private_request,
    cc::mojom::MojoCompositorFrameSinkClientPtr client) {
  DCHECK(frame_sink_manager_ptr_.is_bound());
  frame_sink_manager_ptr_->CreateCompositorFrameSink(
      frame_sink_id, std::move(request), std::move(private_request),
      std::move(client));
}

void FrameSinkManagerHost::RegisterFrameSinkHierarchy(
    const cc::FrameSinkId& parent_frame_sink_id,
    const cc::FrameSinkId& child_frame_sink_id) {
  frame_sink_hiearchy_[child_frame_sink_id] = parent_frame_sink_id;

  DCHECK(frame_sink_manager_ptr_.is_bound());
  frame_sink_manager_ptr_->RegisterFrameSinkHierarchy(parent_frame_sink_id,
                                                      child_frame_sink_id);
}

void FrameSinkManagerHost::UnregisterFrameSinkHierarchy(
    const cc::FrameSinkId& parent_frame_sink_id,
    const cc::FrameSinkId& child_frame_sink_id) {
  auto iter = frame_sink_hiearchy_.find(child_frame_sink_id);
  if (iter != frame_sink_hiearchy_.end()) {
    DCHECK_EQ(iter->second, parent_frame_sink_id);
    frame_sink_hiearchy_.erase(iter);
  }

  DCHECK(frame_sink_manager_ptr_.is_bound());
  frame_sink_manager_ptr_->UnregisterFrameSinkHierarchy(parent_frame_sink_id,
                                                        child_frame_sink_id);
}

void FrameSinkManagerHost::OnSurfaceCreated(
    const cc::SurfaceInfo& surface_info) {
  for (auto& observer : observers_)
    observer.OnSurfaceCreated(surface_info);

  if (!surface_manager()->using_surface_references())
    return;

  // Find the expected embedder for the new surface and assign the temporary
  // reference to it.
  auto iter = frame_sink_hiearchy_.find(surface_info.id().frame_sink_id());
  if (iter != frame_sink_hiearchy_.end()) {
    surface_manager()->AssignTemporaryReference(surface_info.id(),
                                                iter->second);
  } else {
    // No parent for the new surface. Drop the temporary reference, as nothing
    // is expected to embed the new surface.
    surface_manager()->DropTemporaryReference(surface_info.id());
  }
}

}  // namespace viz
