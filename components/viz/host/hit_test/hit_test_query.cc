// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/viz/host/hit_test/hit_test_query.h"

#include "services/viz/public/interfaces/hit_test/hit_test_region_list.mojom.h"

namespace viz {

HitTestQuery::HitTestQuery() = default;

HitTestQuery::~HitTestQuery() = default;

void HitTestQuery::OnSharedMemoryHandlesReceived(
    mojo::ScopedSharedBufferHandle read_handle,
    uint32_t read_size,
    mojo::ScopedSharedBufferHandle write_handle,
    uint32_t write_size) {
  if (!read_handle.is_valid() && !write_handle.is_valid()) {
    SwapHandles();
    return;
  }

  active_handle_ = std::move(read_handle);
  active_handle_size_ = read_size;
  active_mapping_ = active_handle_->Map(active_handle_size_ *
                                        sizeof(AggregatedHitTestRegion));
  idle_handle_ = std::move(write_handle);
  idle_handle_size_ = write_size;
  idle_mapping_ =
      idle_handle_->Map(idle_handle_size_ * sizeof(AggregatedHitTestRegion));
}

Target HitTestQuery::FindTargetForLocation(
    const gfx::Point& location_in_root) const {
  Target target;
  if (!active_handle_size_)
    return target;

  FindTargetInRegionForLocation(
      location_in_root,
      static_cast<AggregatedHitTestRegion*>(active_mapping_.get()), &target);
  return target;
}

bool HitTestQuery::FindTargetInRegionForLocation(
    const gfx::Point& location_in_parent,
    AggregatedHitTestRegion* region,
    Target* target) const {
  gfx::Point location_transformed(location_in_parent);
  region->transform.TransformPoint(&location_transformed);
  if (!region->rect.Contains(location_transformed))
    return false;

  if (region->child_count < 0 ||
      region->child_count >
          (static_cast<AggregatedHitTestRegion*>(active_mapping_.get()) +
           active_handle_size_ - region - 1)) {
    return false;
  }
  AggregatedHitTestRegion* child_region = region + 1;
  AggregatedHitTestRegion* child_region_end =
      child_region + region->child_count;
  gfx::Point location_in_target(location_transformed);
  location_in_target.Offset(-region->rect.x(), -region->rect.y());
  while (child_region < child_region_end) {
    if (FindTargetInRegionForLocation(location_in_target, child_region,
                                      target)) {
      return true;
    }

    if (child_region->child_count < 0 ||
        child_region->child_count >= region->child_count) {
      return false;
    }
    child_region = child_region + child_region->child_count + 1;
  }

  if (region->flags & mojom::kHitTestMine) {
    target->frame_sink_id = region->frame_sink_id;
    target->location_in_target = location_in_target;
    target->flags = region->flags;
    return true;
  }
  return false;
}

void HitTestQuery::SwapHandles() {
  std::swap(active_handle_, idle_handle_);
  std::swap(active_handle_size_, idle_handle_size_);
  std::swap(active_mapping_, idle_mapping_);
}

}  // namespace viz
