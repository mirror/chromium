// Copyright 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/viz/common/quads/aggregated_compositor_frame.h"

namespace viz {

AggregatedCompositorFrame::AggregatedCompositorFrame() = default;

AggregatedCompositorFrame::AggregatedCompositorFrame(AggregatedCompositorFrame&& other) = default;

AggregatedCompositorFrame::~AggregatedCompositorFrame() = default;

AggregatedCompositorFrame& AggregatedCompositorFrame::operator=(AggregatedCompositorFrame&& other) = default;

}  // namespace viz
