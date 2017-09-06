// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_VIZ_SERVICE_HIT_TEST_HIT_TEST_AGGREGATOR_H_
#define COMPONENTS_VIZ_SERVICE_HIT_TEST_HIT_TEST_AGGREGATOR_H_

#include "components/viz/common/hit_test/aggregated_hit_test_region.h"
#include "components/viz/common/surfaces/surface_id.h"
#include "components/viz/service/hit_test/hit_test_manager.h"
#include "components/viz/service/surfaces/surface_observer.h"
#include "components/viz/service/viz_service_export.h"
#include "services/viz/public/interfaces/hit_test/hit_test_region_list.mojom.h"

namespace viz {
class HitTestAggregatorDelegate;

// HitTestAggregator collects HitTestRegionList objects from surfaces and
// aggregates them into a DisplayHitTesData structue made available in
// shared memory to enable efficient hit testing across processes.
//
// This is intended to be created in the viz or GPU process. For mus+ash this
// will be true after the mus process split.
class VIZ_SERVICE_EXPORT HitTestAggregator {
 public:
  // |delegate| owns and outlives HitTestAggregator.
  explicit HitTestAggregator(HitTestManager* hit_test_manager,
                             HitTestAggregatorDelegate* delegate);
  ~HitTestAggregator();

  // Performs the work of Aggregate by creating a PostTask so that
  // the work is not directly on the call.
  void PostTaskAggregate(const SurfaceId& display_surface_id);

  // Called after surfaces have been aggregated into the DisplayFrame.
  // In this call HitTestRegionList structures received from active surfaces
  // are aggregated into the HitTestRegionList structure in
  // shared memory used for event targetting.
  void Aggregate(const SurfaceId& display_surface_id);

  // Called at BeginFrame. Swaps buffers in shared memory and tells its
  // delegate.
  void Swap();

 protected:
  HitTestManager* hit_test_manager_;

  // Keeps track of the number of regions in the active list
  // so that we know when we exceed the available length.
  uint32_t active_region_count_ = 0;

  mojo::ScopedSharedBufferHandle read_handle_;
  mojo::ScopedSharedBufferHandle write_handle_;

  // The number of elements allocated.
  uint32_t read_size_ = 0;
  uint32_t write_size_ = 0;

  mojo::ScopedSharedBufferMapping read_buffer_;
  mojo::ScopedSharedBufferMapping write_buffer_;

  bool handle_replaced_ = false;

  // Can only be 0 or 1 when we only have two buffers.
  uint8_t active_handle_index_ = 0;

  HitTestAggregatorDelegate* const delegate_;

 private:
  // Allocates memory for the AggregatedHitTestRegion array.
  void AllocateHitTestRegionArray();
  void AllocateHitTestRegionArray(uint32_t length);

  void SwapHandles();

  // Appends the root element to the AggregatedHitTestRegion array.
  void AppendRoot(const SurfaceId& surface_id);

  // Appends a region to the HitTestRegionList structure to recursively
  // build the tree.
  size_t AppendRegion(AggregatedHitTestRegion* regions,
                      size_t region_index,
                      const mojom::HitTestRegionPtr& region);

  // Handles the case when this object is deleted after
  // the PostTaskAggregation call is scheduled but before invocation.
  base::WeakPtrFactory<HitTestAggregator> weak_ptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(HitTestAggregator);
};

}  // namespace viz

#endif  // COMPONENTS_VIZ_SERVICE_HIT_TEST_HIT_TEST_AGGREGATOR_H_
