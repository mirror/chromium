// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/viz/service/hit_test/hit_test_aggregator.h"

#include "components/viz/common/hit_test/aggregated_hit_test_region.h"
#include "components/viz/service/frame_sinks/frame_sink_manager_impl.h"
#include "components/viz/service/hit_test/hit_test_aggregator_delegate.h"

namespace viz {

namespace {
// TODO(gklassen): Review and select appropriate sizes based on
// telemetry / UMA.
constexpr uint32_t kMaxRegionsPerSurface = 1024;
}  // namespace

HitTestManager::HitTestManager(FrameSinkManagerImpl* frame_sink_manager)
    : frame_sink_manager_(frame_sink_manager) {}

HitTestManager::~HitTestManager() = default;

void HitTestManager::SubmitHitTestRegionList(
    const SurfaceId& surface_id,
    uint64_t frame_index,
    mojom::HitTestRegionListPtr hit_test_region_list) {
  if (!ValidateHitTestRegionList(surface_id, hit_test_region_list))
    return;
  // TODO(gklassen): Runtime validation that hit_test_region_list is valid.
  // TODO(gklassen): Inform FrameSink that the hit_test_region_list is invalid.
  // TODO(gklassen): FrameSink needs to inform the host of a difficult renderer.
  hit_test_region_lists_[surface_id] = std::move(hit_test_region_list);
}

bool HitTestManager::ValidateHitTestRegionList(
    const SurfaceId& surface_id,
    const mojom::HitTestRegionListPtr& hit_test_region_list) {
  if (!hit_test_region_list)
    return false;
  if (hit_test_region_list->regions.size() > kMaxRegionsPerSurface)
    return false;
  for (auto& region : hit_test_region_list->regions) {
    if (!ValidateHitTestRegion(surface_id, region))
      return false;
  }
  return true;
}

bool HitTestManager::ValidateHitTestRegion(
    const SurfaceId& surface_id,
    const mojom::HitTestRegionPtr& hit_test_region) {
  // If client_id is 0 then use the client_id that
  // matches the compositor frame.
  if (hit_test_region->frame_sink_id.client_id() == 0) {
    hit_test_region->frame_sink_id =
        FrameSinkId(surface_id.frame_sink_id().client_id(),
                    hit_test_region->frame_sink_id.sink_id());
  }
  // TODO(gklassen): Ensure that |region->frame_sink_id| is a child of
  // |frame_sink_id|.
  if (hit_test_region->flags & mojom::kHitTestChildSurface) {
    if (!hit_test_region->local_surface_id.has_value() ||
        !hit_test_region->local_surface_id->is_valid())
      return false;
  }
  return true;
}

bool HitTestManager::OnSurfaceDamaged(const SurfaceId& surface_id,
                                      const BeginFrameAck& ack) {
  return false;
}

void HitTestManager::OnSurfaceDiscarded(const SurfaceId& surface_id) {
  hit_test_region_lists_.erase(surface_id);
}

const mojom::HitTestRegionList* HitTestManager::GetHitTestRegionList(
    const SurfaceId& surface_id) {
  auto search = hit_test_region_lists_.find(surface_id);
  if (search == hit_test_region_lists_.end()) {
    return nullptr;
  }
  return search->second.get();
}

}  // namespace viz
