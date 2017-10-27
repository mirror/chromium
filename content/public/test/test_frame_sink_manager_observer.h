// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_PUBLIC_TEST_TEST_FRAME_SINK_MANAGER_OBSERVER_H_
#define CONTENT_PUBLIC_TEST_TEST_FRAME_SINK_MANAGER_OBSERVER_H_

#include "mojo/public/cpp/bindings/binding.h"
#include "services/viz/privileged/interfaces/compositing/frame_sink_manager.mojom.h"

namespace content {
class RenderFrameHost;
class RenderWidgetHostViewBase;

// Allows for the observation of changes within
// viz::mojom::FrameSinkManagerClient. This will not be notified of changes
// which have occurred before observation began. So it should be instantied at
// the start of a test.
//
// This will track all surfaces activated during a test.
class TestFrameSinkManagerObserver
    : public viz::mojom::FrameSinkManagerTestObserver {
 public:
  TestFrameSinkManagerObserver();
  ~TestFrameSinkManagerObserver() override;

  // Waits until the first CompositorFrame with the |child_frame|'s SurfaceId
  // activates. Will return immediately if activation occurred previously.
  void WaitForChildFrameSurfaceReady(content::RenderFrameHost* child_frame);

 private:
  // Waits until the first CompositorFrame with the |root_container|'s SurfaceId
  // activates. Will return immediately if activation occurred previously.
  void WaitForSurfaceReady(RenderWidgetHostViewBase* root_container);

  // mojom::FrameSinkManagerObserver:
  void OnFirstSurfaceActivation(const viz::SurfaceInfo& surface_info) override;

  // All surfaces which have been activated.
  std::map<viz::FrameSinkId, viz::SurfaceInfo> surfaces_;

  viz::mojom::FrameSinkManagerTestConnectorPtr
      frame_sink_manager_test_connector_ptr_;
  mojo::Binding<viz::mojom::FrameSinkManagerTestObserver> binding_;

  DISALLOW_COPY_AND_ASSIGN(TestFrameSinkManagerObserver);
};

}  // namespace content

#endif  // CONTENT_PUBLIC_TEST_TEST_FRAME_SINK_MANAGER_OBSERVER_H_
