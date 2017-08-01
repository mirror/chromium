// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/viz/host/hit_test/hit_test_query.h"

#include "services/viz/public/interfaces/hit_test/hit_test_region_list.mojom.h"

namespace viz {

HitTestQuery::HitTestQuery() = default;

HitTestQuery::~HitTestQuery() = default;

void HitTestQuery::OnAggregatedHitTestRegionListUpdated(
    mojo::ScopedSharedBufferHandle handle_one,
    uint32_t handle_one_size,
    mojo::ScopedSharedBufferHandle handle_two,
    uint32_t handle_two_size,
    bool use_handle_one) {
  if (handle_one.is_valid()) {
    handle_one_size_ = handle_one_size;
    handle_one_buffer_ =
        handle_one->Map(handle_one_size_ * sizeof(AggregatedHitTestRegion));
  }

  if (handle_two.is_valid()) {
    handle_two_size_ = handle_two_size;
    handle_two_buffer_ =
        handle_two->Map(handle_two_size_ * sizeof(AggregatedHitTestRegion));
  }

  if (!handle_one_buffer_ || !handle_two_buffer_) {
    // TODO(riajiang): Report security fault. http://crbug.com/746470
    return;
  }

  if (use_handle_one) {
    active_hit_test_list_ =
        static_cast<AggregatedHitTestRegion*>(handle_one_buffer_.get());
    active_hit_test_list_size_ = handle_one_size_;
  } else {
    active_hit_test_list_ =
        static_cast<AggregatedHitTestRegion*>(handle_two_buffer_.get());
    active_hit_test_list_size_ = handle_two_size_;
  }
}

Target HitTestQuery::FindTargetForLocation(
    const gfx::Point& location_in_root) const {
  Target target;
  if (!active_hit_test_list_size_)
    return target;

  FindTargetInRegionForLocation(location_in_root, active_hit_test_list_,
                                &target);
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
          (active_hit_test_list_ + active_hit_test_list_size_ - region - 1)) {
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

}  // namespace viz
