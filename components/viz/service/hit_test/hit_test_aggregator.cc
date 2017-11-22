// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/viz/service/hit_test/hit_test_aggregator.h"

#include "components/viz/common/hit_test/aggregated_hit_test_region.h"
#include "components/viz/service/hit_test/hit_test_aggregator_delegate.h"
#include "third_party/skia/include/core/SkMatrix44.h"

namespace viz {

namespace {
// TODO(gklassen): Review and select appropriate sizes based on
// telemetry / UMA.
constexpr uint32_t kInitialSize = 1024;
constexpr uint32_t kIncrementalSize = 1024;
// constexpr uint32_t kMaxSize = 100 * 1024;

void PrepareTransformForReadOnlySharedMemory(gfx::Transform* transform) {
  // |transform| is going to be shared in read-only memory to HitTestQuery.
  // However, if HitTestQuery tries to operate on it, then it is possible that
  // it will attempt to perform write on the underlying SkMatrix44 [1], causing
  // invalid memory write in read-only memory.
  // [1]
  // https://cs.chromium.org/chromium/src/third_party/skia/include/core/SkMatrix44.h?l=133
  // Explicitly calling getType() to compute the type-mask in SkMatrix44.
  transform->matrix().getType();
}

}  // namespace

HitTestAggregator::HitTestAggregator(const HitTestManager* hit_test_manager,
                                     HitTestAggregatorDelegate* delegate)
    : hit_test_manager_(hit_test_manager),
      delegate_(delegate),
      weak_ptr_factory_(this) {
  AllocateHitTestRegionArray();
}

HitTestAggregator::~HitTestAggregator() = default;

void HitTestAggregator::Aggregate(const SurfaceId& display_surface_id) {
  AppendRoot(display_surface_id);
  Swap();
}

void HitTestAggregator::GrowRegionList() {
  ResizeHitTestRegionArray(write_size_ + kIncrementalSize);
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
  ResizeHitTestRegionArray(kInitialSize);
  SwapHandles();
  ResizeHitTestRegionArray(kInitialSize);
}

void HitTestAggregator::ResizeHitTestRegionArray(uint32_t size) {
  size_t num_bytes =
      size * (sizeof(AggregatedHitTestRegion) + sizeof(HitTestRect));
  write_handle_ = mojo::SharedBufferHandle::Create(num_bytes);
  auto new_buffer = write_handle_->Map(num_bytes);
  handle_replaced_ = true;

  AggregatedHitTestRegion* region = (AggregatedHitTestRegion*)new_buffer.get();
  if (write_size_)
    memcpy(region, write_buffer_.get(), write_size_);
  else
    region->child_count = kEndOfList;

  write_size_ = size;
  write_buffer_ = std::move(new_buffer);
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
      hit_test_manager_->GetActiveHitTestRegionList(surface_id);
  if (!hit_test_region_list)
    return;

  std::vector<HitTestRect> hit_test_rects;
  hit_test_rects.push_back(
      HitTestRect(mojom::kHitTestMouse | mojom::kHitTestTouch,
                  hit_test_region_list->bounds));

  AggregatedHitTestRegion* regions =
      static_cast<AggregatedHitTestRegion*>(write_buffer_.get());
  AggregatedHitTestRegion* region_start =
      reinterpret_cast<AggregatedHitTestRegion*>(
          (char*)regions + sizeof(AggregatedHitTestRegion) +
          sizeof(HitTestRect) * hit_test_rects.size());
  int32_t child_count = 0;
  for (const auto& region : hit_test_region_list->regions) {
    if (region_start)
      // if (region_index >= write_size_ - 1)
      //   break;
      NextRegionStatus next_region_status = AppendRegion(region_start, region);
    region_start = next_region_status.next_region;
    child_count += next_region_status.child_count_so_far;
    LOG(ERROR) << region_start;
  }

  SetRegionAt(regions, surface_id.frame_sink_id(), hit_test_region_list->flags,
              hit_test_region_list->transform, std::move(hit_test_rects),
              child_count);
  MarkEndAt(region_start);
}

HitTestAggregator::NextRegionStatus HitTestAggregator::AppendRegion(
    AggregatedHitTestRegion* region_ptr,
    const mojom::HitTestRegionPtr& region) {
  // if (region_index >= write_size_ - 1) {
  //   if (write_size_ > kMaxSize) {
  //     MarkEndAt(parent_index);
  //     return region_index;
  //   } else {
  //     GrowRegionList();
  //   }
  // }

  uint32_t flags = region->flags;
  gfx::Transform transform = region->transform;

  std::vector<HitTestRect> hit_test_rects;
  for (const auto& hit_test_rect : region->rects) {
    hit_test_rects.push_back(
        HitTestRect(hit_test_rect->flags, hit_test_rect->rect));
  }

  int32_t child_count = 0;
  AggregatedHitTestRegion* region_start = region_ptr;
  if (region->flags & mojom::kHitTestChildSurface) {
    auto surface_id =
        SurfaceId(region->frame_sink_id, region->local_surface_id.value());
    const mojom::HitTestRegionList* hit_test_region_list =
        hit_test_manager_->GetActiveHitTestRegionList(surface_id);
    if (!hit_test_region_list) {
      // Surface HitTestRegionList not found - it may be late.
      // In this case, this child surface doesn't have any children regions,
      // so it should accept events itself. E.g. when renderer doesn't submit
      // any hit-test data for its children, itself should still be able to
      // get events.
      flags |= mojom::kHitTestMine;
    } else {
      // Rather than add a node in the tree for this hit_test_region_list
      // element we can simplify the tree by merging the flags and transform
      // into the kHitTestChildSurface element.
      if (!hit_test_region_list->transform.IsIdentity())
        transform.PreconcatTransform(hit_test_region_list->transform);

      flags |= hit_test_region_list->flags;

      region_start = reinterpret_cast<AggregatedHitTestRegion*>(
          (char*)region_ptr + sizeof(AggregatedHitTestRegion) +
          sizeof(HitTestRect) * hit_test_rects.size());
      for (const auto& child_region : hit_test_region_list->regions) {
        NextRegionStatus next_region_status =
            AppendRegion(region_start, child_region);
        // if (region_index >= write_size_ - 1)
        //   break;
        region_start = next_region_status.next_region;
        child_count += next_region_status.child_count_so_far;

        LOG(ERROR) << region_start;
      }
    }
  }

  NextRegionStatus next_region_status;
  AggregatedHitTestRegion* region_ptr_next =
      SetRegionAt(region_ptr, region->frame_sink_id, flags, transform,
                  std::move(hit_test_rects), child_count);
  if (region_start == region_ptr) {
    next_region_status.next_region = region_ptr_next;
  } else {
    next_region_status.next_region = region_start;
  }
  next_region_status.child_count_so_far = child_count + 1;
  return next_region_status;
}

AggregatedHitTestRegion* HitTestAggregator::SetRegionAt(
    AggregatedHitTestRegion* region,
    const FrameSinkId& frame_sink_id,
    uint32_t flags,
    const gfx::Transform& transform,
    std::vector<HitTestRect> rects,
    int32_t child_count) {
  region->frame_sink_id = frame_sink_id;
  LOG(ERROR) << region << " " << region->frame_sink_id << child_count;
  region->flags = flags;
  region->transform = transform;
  region->child_count = child_count;
  region->num_rects = rects.size();

  HitTestRect* hit_test_rect = reinterpret_cast<HitTestRect*>(region + 1);
  for (const auto& rect : rects) {
    hit_test_rect->flags = rect.flags;
    hit_test_rect->rect = rect.rect;
    hit_test_rect++;
  }

  PrepareTransformForReadOnlySharedMemory(&region->transform);

  return reinterpret_cast<AggregatedHitTestRegion*>(hit_test_rect);
}

void HitTestAggregator::MarkEndAt(AggregatedHitTestRegion* region_end) {
  region_end->child_count = kEndOfList;
}

}  // namespace viz
