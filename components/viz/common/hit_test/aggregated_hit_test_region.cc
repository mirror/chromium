// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/viz/common/hit_test/aggregated_hit_test_region.h"

namespace viz {

AggregatedHitTestRegion::AggregatedHitTestRegion(
    FrameSinkId frame_sink_id,
    uint32_t flags,
    gfx::Transform transform,
    std::vector<mojom::HitTestRectPtr> rects,
    int32_t child_count)
    : frame_sink_id(frame_sink_id),
      flags(flags),
      transform(transform),
      rects(std::move(rects)),
      child_count(child_count) {}

AggregatedHitTestRegion::~AggregatedHitTestRegion() {}

}  // namespace viz
