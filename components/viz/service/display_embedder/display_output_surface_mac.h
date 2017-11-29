// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_VIZ_SERVICE_DISPLAY_EMBEDDER_DISPLAY_OUTPUT_SURFACE_MAC_H_
#define COMPONENTS_VIZ_SERVICE_DISPLAY_EMBEDDER_DISPLAY_OUTPUT_SURFACE_MAC_H_

#include "components/viz/service/display_embedder/display_output_surface.h"

namespace viz {

// TODO(ccameron): This should share a common base class with Ozone.
class DisplayOutputSurfaceMac : public DisplayOutputSurface {
 public:
  DisplayOutputSurfaceMac(
      scoped_refptr<InProcessContextProvider> context_provider,
      SyntheticBeginFrameSource* synthetic_begin_frame_source);
  ~DisplayOutputSurfaceMac() override;

 private:
  // DisplayOutputSurface:
  void DidReceiveCALayerParams(
      const gpu::GpuProcessHostedCALayerTreeParamsMac* params_mac) override;
  void DidReceiveSwapBuffersAck(gfx::SwapResult result) override;

  DISALLOW_COPY_AND_ASSIGN(DisplayOutputSurfaceMac);
};

}  // namespace viz

#endif  // COMPONENTS_VIZ_SERVICE_DISPLAY_EMBEDDER_DISPLAY_OUTPUT_SURFACE_MAC_H_
