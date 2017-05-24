// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "modules/vr/latest/VRDevicePose.h"

#include "modules/vr/latest/VRView.h"

namespace blink {

VRDevicePose::VRDevicePose(DOMFloat32Array* pose_model_matrix)
    : pose_model_matrix_(pose_model_matrix) {}

DOMFloat32Array* VRDevicePose::getViewMatrix(VRView* view) {
  // TODO(bajones): Return the appropriate matrix for the view
  return nullptr;
}

DEFINE_TRACE(VRDevicePose) {
  visitor->Trace(pose_model_matrix_);
}

}  // namespace blink
