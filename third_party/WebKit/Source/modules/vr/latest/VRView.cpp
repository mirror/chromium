// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "modules/vr/latest/VRView.h"

#include "modules/vr/latest/VRLayer.h"
#include "modules/vr/latest/VRViewport.h"

namespace blink {

VRView::VRView(const String& eye, DOMFloat32Array* projection_matrix)
    : eye_(eye), projection_matrix_(projection_matrix) {}

VRViewport* VRView::getViewport(VRLayer* layer) const {
  // TODO(bajones): Return the appropriate viewport
  return nullptr;
}

DEFINE_TRACE(VRView) {
  visitor->Trace(projection_matrix_);
}

}  // namespace blink
