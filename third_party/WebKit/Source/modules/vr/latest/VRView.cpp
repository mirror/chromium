// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "modules/vr/latest/VRView.h"

#include "modules/vr/latest/VRLayer.h"
#include "modules/vr/latest/VRPresentationFrame.h"
#include "modules/vr/latest/VRViewport.h"

namespace blink {

VRView::VRView(VRPresentationFrame* frame, Eye eye)
    : eye_(eye),
      frame_(frame),
      projection_matrix_(DOMFloat32Array::Create(16)) {
  eye_string_ = (eye_ == kEyeLeft ? "left" : "right");
}

VRSession* VRView::session() const {
  return frame_->session();
}

VRViewport* VRView::getViewport(VRLayer* layer) const {
  if (!layer || layer->session() != frame_->session())
    return nullptr;

  return layer->GetViewport(eye_);
}

void VRView::UpdateProjectionMatrix(float up_rad,
                                    float down_rad,
                                    float left_rad,
                                    float right_rad,
                                    float near,
                                    float far) {
  float up_tan = tanf(up_rad);
  float down_tan = tanf(down_rad);
  float left_tan = tanf(left_rad);
  float right_tan = tanf(right_rad);
  float x_scale = 2.0f / (left_tan + right_tan);
  float y_scale = 2.0f / (up_tan + down_tan);

  float* out = projection_matrix_->Data();
  out[0] = x_scale;
  out[1] = 0.0f;
  out[2] = 0.0f;
  out[3] = 0.0f;
  out[4] = 0.0f;
  out[5] = y_scale;
  out[6] = 0.0f;
  out[7] = 0.0f;
  out[8] = -((left_tan - right_tan) * x_scale * 0.5);
  out[9] = ((up_tan - down_tan) * y_scale * 0.5);
  out[10] = (near + far) / (near - far);
  out[11] = -1.0f;
  out[12] = 0.0f;
  out[13] = 0.0f;
  out[14] = (2 * far * near) / (near - far);
  out[15] = 0.0f;
}

void VRView::UpdateOffset(float x, float y, float z) {
  offset_.Set(x, y, z);
}

DEFINE_TRACE(VRView) {
  visitor->Trace(frame_);
  visitor->Trace(projection_matrix_);
}

}  // namespace blink
