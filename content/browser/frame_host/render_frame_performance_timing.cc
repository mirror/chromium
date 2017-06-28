// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/frame_host/render_frame_performance_timing.h"

#include <utility>

#include "base/logging.h"
#include "content/browser/frame_host/render_frame_host_delegate.h"
#include "content/browser/frame_host/render_frame_host_impl.h"

namespace content {

namespace {

const char kRenderFramePerformanceTimingIdentifierKey[] =
    "RenderFramePerformanceTimingIdentifierKey";

}  // namespace

RenderFramePerformanceTiming::RenderFramePerformanceTiming(
    RenderFrameHostImpl* render_frame_host,
    mojo::InterfaceRequest<blink::mojom::PerformanceTiming> request)
    : render_frame_host_(render_frame_host),
      binding_(this, std::move(request)) {}

RenderFramePerformanceTiming::~RenderFramePerformanceTiming() = default;

void RenderFramePerformanceTiming::OnFirstPaint(
    base::TimeDelta time_to_first_paint) {
  render_frame_host_->delegate()->OnFirstPaintInFrame(render_frame_host_,
                                                      time_to_first_paint);
}

// static
void RenderFramePerformanceTiming::CreateService(
    RenderFrameHostImpl* render_frame_host,
    const service_manager::BindSourceInfo& source_info,
    blink::mojom::PerformanceTimingRequest request) {
  auto performance_timing = base::MakeUnique<RenderFramePerformanceTiming>(
      render_frame_host, std::move(request));
  render_frame_host->SetUserData(kRenderFramePerformanceTimingIdentifierKey,
                                 std::move(performance_timing));
}

}  // namespace content
