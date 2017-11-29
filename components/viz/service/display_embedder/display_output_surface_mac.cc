// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/viz/service/display_embedder/display_output_surface_mac.h"

#include "components/viz/service/display/output_surface_client.h"
#include "gpu/ipc/client/gpu_process_hosted_ca_layer_tree_params.h"

namespace viz {

DisplayOutputSurfaceMac::DisplayOutputSurfaceMac(
    scoped_refptr<InProcessContextProvider> context_provider,
    SyntheticBeginFrameSource* synthetic_begin_frame_source)
    : DisplayOutputSurface(context_provider, synthetic_begin_frame_source) {}

DisplayOutputSurfaceMac::~DisplayOutputSurfaceMac() {}

void DisplayOutputSurfaceMac::DidReceiveSwapDisplayParams(
    const gpu::GpuProcessHostedCALayerTreeParamsMac* params_mac) {
  client()->DidReceiveTextureInUseResponses(params_mac->responses);
}

void DisplayOutputSurfaceMac::DidReceiveSwapBuffersAck(gfx::SwapResult result) {
  DisplayOutputSurface::DidReceiveSwapBuffersAck(result);
}

}  // namespace viz
