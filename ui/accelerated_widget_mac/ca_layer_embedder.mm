// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/accelerated_widget_mac/ca_layer_embedder.h"

#include "ui/accelerated_widget_mac/accelerated_widget_mac.h"

namespace ui {

// static
CALayerEmbedder* CALayerEmbedder::FromAcceleratedWidget(
    gfx::AcceleratedWidget widget) {
  return AcceleratedWidgetMac::Get(widget);
}

}  // namespace ui
