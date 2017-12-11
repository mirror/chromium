// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/compositor/in_process_display_private_client.h"
#include "build/build_config.h"

#if defined(OS_MACOSX)
#include "ui/accelerated_widget_mac/ca_layer_frame_sink.h"
#endif

namespace content {

InProcessDisplayPrivateClient::InProcessDisplayPrivateClient(
    gfx::AcceleratedWidget widget)
    : binding_(this), widget_(widget) {}

InProcessDisplayPrivateClient::~InProcessDisplayPrivateClient() {}

viz::mojom::DisplayPrivateClientPtr
InProcessDisplayPrivateClient::GetBoundPtr() {
  viz::mojom::DisplayPrivateClientPtr ptr;
  binding_.Bind(mojo::MakeRequest(&ptr));
  return ptr;
}

void InProcessDisplayPrivateClient::OnDisplayReceivedCALayerParams(
    const gfx::CALayerParams& ca_layer_params) {
#if defined(OS_MACOSX)
  ui::CALayerFrameSink* ca_layer_frame_sink =
      ui::CALayerFrameSink::FromAcceleratedWidget(widget_);
  if (ca_layer_frame_sink)
    ca_layer_frame_sink->UpdateCALayerTree(ca_layer_params);
  else
    DLOG(WARNING) << "Received frame for non-existent widget.";
#else
  DLOG(ERROR) << "Should not receive CALayer params on non-macOS platforms.";
#endif
}

}  // namespace content
