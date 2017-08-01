// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_VIZ_SERVICE_HIT_TEST_HIT_TEST_AGGREGATOR_DELEGATE_H_
#define COMPONENTS_VIZ_SERVICE_HIT_TEST_HIT_TEST_AGGREGATOR_DELEGATE_H_

namespace viz {
// Used by HitTestAggregator to talk to GpuRootCompositorFrameSink.
class HitTestAggregatorDelegate {
 public:
  // Notifies GpuRootCompositorFrameSink to update ScopedSharedBufferHandles of
  // its client.
  virtual void OnAggregatedHitTestRegionListUpdated(
      mojo::ScopedSharedBufferHandle handle_one,
      uint32_t handle_one_size,
      mojo::ScopedSharedBufferHandle handle_two,
      uint32_t handle_two_size,
      bool use_handle_one) = 0;

 protected:
  virtual ~HitTestAggregatorDelegate() {}
};

}  // namespace viz

#endif  // COMPONENTS_VIZ_SERVICE_HIT_TEST_HIT_TEST_AGGREGATOR_DELEGATE_H_
