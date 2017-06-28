// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_FRAME_HOST_RENDER_FRAME_PERFORMANCE_TIMING_H_
#define CONTENT_BROWSER_FRAME_HOST_RENDER_FRAME_PERFORMANCE_TIMING_H_

#include "base/macros.h"
#include "base/supports_user_data.h"
#include "mojo/public/cpp/bindings/binding.h"
#include "third_party/WebKit/public/platform/performance_timing.mojom.h"

namespace service_manager {
struct BindSourceInfo;
}  // namespace service_manager

namespace content {

class RenderFrameHostImpl;

class RenderFramePerformanceTiming : public base::SupportsUserData::Data,
                                     public blink::mojom::PerformanceTiming {
 public:
  RenderFramePerformanceTiming(
      RenderFrameHostImpl* render_frame_host,
      mojo::InterfaceRequest<blink::mojom::PerformanceTiming> request);
  ~RenderFramePerformanceTiming() override;

  // mojom::PerformanceTiming
  void OnFirstPaint(base::TimeDelta time_to_first_paint) override;

  static void CreateService(RenderFrameHostImpl* render_frame_host,
                            const service_manager::BindSourceInfo& source_info,
                            blink::mojom::PerformanceTimingRequest request);

 private:
  RenderFrameHostImpl* render_frame_host_;
  mojo::Binding<blink::mojom::PerformanceTiming> binding_;

  DISALLOW_COPY_AND_ASSIGN(RenderFramePerformanceTiming);
};

}  // namespace content

#endif  // CONTENT_BROWSER_FRAME_HOST_RENDER_FRAME_PERFORMANCE_TIMING_H_
