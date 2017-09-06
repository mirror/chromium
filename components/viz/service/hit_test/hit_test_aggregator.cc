// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/viz/service/hit_test/hit_test_aggregator.h"

#include "components/viz/common/hit_test/aggregated_hit_test_region.h"
#include "components/viz/service/hit_test/hit_test_aggregator_delegate.h"

namespace viz {

namespace {
// TODO(gklassen): Review and select appropriate sizes based on
// telemetry / UMA.
constexpr uint32_t kInitialSize = 1024;
// constexpr uint32_t kIncrementalSize = 1024;
// constexpr uint32_t kMaxSize = 100 * 1024;

}  // namespace

HitTestAggregator::HitTestAggregator(HitTestManager* hit_test_manager,
                                     HitTestAggregatorDelegate* delegate)
    : hit_test_manager_(hit_test_manager),
      delegate_(delegate),
      weak_ptr_factory_(this) {
  AllocateHitTestRegionArray();
}

HitTestAggregator::~HitTestAggregator() = default;

void HitTestAggregator::PostTaskAggregate(const SurfaceId& display_surface_id) {
  base::ThreadTaskRunnerHandle::Get()->PostTask(
      FROM_HERE,
      base::BindOnce(&HitTestAggregator::Aggregate,
                     weak_ptr_factory_.GetWeakPtr(), display_surface_id));
}

void HitTestAggregator::Aggregate(const SurfaceId& display_surface_id) {
  // TODO(gklassen):
  /*
  // Check to ensure that enough memory has been allocated.
  uint32_t size = write_size_;
  uint32_t max_size = active_region_count_ + active_.size() + 1;
  if (max_size > kMaxSize)
    max_size = kMaxSize;

  if (max_size > size) {
    size = (1 + max_size / kIncrementalSize) * kIncrementalSize;
    AllocateHitTestRegionArray(size);
  }
  */
  AppendRoot(display_surface_id);

  // TODO(gklassen): Invoke Swap at BeginFrame and remove this call.
  Swap();
}

void HitTestAggregator::Swap() {
  SwapHandles();
  if (!handle_replaced_) {
    delegate_->SwitchActiveAggregatedHitTestRegionList(active_handle_index_);
    return;
  }

  delegate_->OnAggregatedHitTestRegionListUpdated(
      read_handle_->Clone(mojo::SharedBufferHandle::AccessMode::READ_ONLY),
      read_size_,
      write_handle_->Clone(mojo::SharedBufferHandle::AccessMode::READ_ONLY),
      write_size_);
  active_handle_index_ = 0;
  handle_replaced_ = false;
}

void HitTestAggregator::AllocateHitTestRegionArray() {
  AllocateHitTestRegionArray(kInitialSize);
  SwapHandles();
  AllocateHitTestRegionArray(kInitialSize);
}

void HitTestAggregator::AllocateHitTestRegionArray(uint32_t size) {
  size_t num_bytes = size * sizeof(AggregatedHitTestRegion);
  write_handle_ = mojo::SharedBufferHandle::Create(num_bytes);
  write_size_ = size;
  write_buffer_ = write_handle_->Map(num_bytes);
  handle_replaced_ = true;

  AggregatedHitTestRegion* region =
      (AggregatedHitTestRegion*)write_buffer_.get();
  region[0].child_count = kEndOfList;
}

void HitTestAggregator::SwapHandles() {
  using std::swap;

  swap(read_handle_, write_handle_);
  swap(read_size_, write_size_);
  swap(read_buffer_, write_buffer_);
  active_handle_index_ = !active_handle_index_;
}

void HitTestAggregator::AppendRoot(const SurfaceId& surface_id) {
  const mojom::HitTestRegionList* hit_test_region_list =
      hit_test_manager_->GetHitTestRegionList(surface_id);

  if (!hit_test_region_list)
    return;

  AggregatedHitTestRegion* regions =
      static_cast<AggregatedHitTestRegion*>(write_buffer_.get());

  regions[0].frame_sink_id = surface_id.frame_sink_id();
  regions[0].flags = hit_test_region_list->flags;
  regions[0].rect = hit_test_region_list->bounds;
  regions[0].transform = hit_test_region_list->transform;

  size_t region_index = 1;
  for (const auto& region : hit_test_region_list->regions) {
    if (region_index >= write_size_ - 1)
      break;
    region_index = AppendRegion(regions, region_index, region);
  }

  DCHECK_GE(region_index, 1u);
  regions[0].child_count = region_index - 1;
  regions[region_index].child_count = kEndOfList;
}

size_t HitTestAggregator::AppendRegion(AggregatedHitTestRegion* regions,
                                       size_t region_index,
                                       const mojom::HitTestRegionPtr& region) {
  AggregatedHitTestRegion* element = &regions[region_index];

  element->frame_sink_id = region->frame_sink_id;
  element->flags = region->flags;
  element->rect = region->rect;
  element->transform = region->transform;

  size_t parent_index = region_index++;
  if (region_index >= write_size_ - 1) {
    element->child_count = 0;
    return region_index;
  }

  if (region->flags & mojom::kHitTestChildSurface) {
    auto surface_id =
        SurfaceId(region->frame_sink_id, region->local_surface_id.value());
    const mojom::HitTestRegionList* hit_test_region_list =
        hit_test_manager_->GetHitTestRegionList(surface_id);
    if (!hit_test_region_list) {
      // Surface HitTestRegionList not found - it may be late.
      // Don't include this region so that it doesn't receive events.
      return parent_index;
    }

    // Rather than add a node in the tree for this hit_test_region_list element
    // we can simplify the tree by merging the flags and transform into
    // the kHitTestChildSurface element.
    if (!hit_test_region_list->transform.IsIdentity())
      element->transform.PreconcatTransform(hit_test_region_list->transform);

    element->flags |= hit_test_region_list->flags;

    for (const auto& child_region : hit_test_region_list->regions) {
      region_index = AppendRegion(regions, region_index, child_region);
      if (region_index >= write_size_ - 1)
        break;
    }
  }
  DCHECK_GE(region_index - parent_index - 1, 0u);
  element->child_count = region_index - parent_index - 1;
  return region_index;
}

}  // namespace viz
