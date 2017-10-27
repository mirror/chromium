// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/viz/service/frame_sinks/frame_sink_manager_test_connector_impl.h"

#include "components/viz/service/frame_sinks/frame_sink_manager_impl.h"

namespace viz {

// static
void FrameSinkManagerTestConnectorImpl::CreateAndBind(
    FrameSinkManagerImpl* manager,
    mojom::FrameSinkManagerTestConnectorRequest request) {
  manager->test_connector_impl_ =
      base::MakeUnique<FrameSinkManagerTestConnectorImpl>(std::move(request));
}

FrameSinkManagerTestConnectorImpl::FrameSinkManagerTestConnectorImpl(
    mojom::FrameSinkManagerTestConnectorRequest request)
    : binding_(this) {
  binding_.Bind(std::move(request));
}

FrameSinkManagerTestConnectorImpl::~FrameSinkManagerTestConnectorImpl() {}

void FrameSinkManagerTestConnectorImpl::OnFirstSurfaceActivation(
    const SurfaceInfo& surface_info) {
  observers_.ForAllPtrs(
      [this, surface_info](mojom::FrameSinkManagerTestObserver* observer) {
        observer->OnFirstSurfaceActivation(surface_info);
      });
}

void FrameSinkManagerTestConnectorImpl::AddObserver(
    mojom::FrameSinkManagerTestObserverPtr observer) {
  observers_.AddPtr(std::move(observer));
}

}  // namespace viz
