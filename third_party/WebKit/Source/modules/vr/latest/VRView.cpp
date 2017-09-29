// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "modules/vr/latest/VRView.h"

#include "modules/vr/latest/VRLayer.h"
#include "modules/vr/latest/VRPresentationFrame.h"
#include "modules/vr/latest/VRViewport.h"
#include "platform/geometry/FloatPoint3D.h"

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

void VRView::UpdateProjectionMatrixFromFoV(float up_rad,
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
  float nf = 1.0f / (near - far);

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
  out[10] = (near + far) * nf;
  out[11] = -1.0f;
  out[12] = 0.0f;
  out[13] = 0.0f;
  out[14] = (2.0f * far * near) * nf;
  out[15] = 0.0f;

  inv_projection_dirty_ = true;
}

void VRView::UpdateProjectionMatrixFromAspect(float fovy,
                                              float aspect,
                                              float near,
                                              float far) {
  float f = 1.0f / tanf(fovy / 2);
  float nf = 1.0f / (near - far);

  float* out = projection_matrix_->Data();
  out[0] = f / aspect;
  out[1] = 0.0f;
  out[2] = 0.0f;
  out[3] = 0.0f;
  out[4] = 0.0f;
  out[5] = f;
  out[6] = 0.0f;
  out[7] = 0.0f;
  out[8] = 0.0f;
  out[9] = 0.0f;
  out[10] = (far + near) * nf;
  out[11] = -1.0f;
  out[12] = 0.0f;
  out[13] = 0.0f;
  out[14] = (2.0f * far * near) * nf;
  out[15] = 0.0f;

  inv_projection_dirty_ = true;
}

void VRView::UpdateOffset(float x, float y, float z) {
  offset_.Set(x, y, z);
}

std::unique_ptr<TransformationMatrix> VRView::unprojectPointerTransform(
    double x,
    double y,
    double width,
    double height) {
  // Recompute the inverse projection matrix if needed.
  if (inv_projection_dirty_) {
    float* m = projection_matrix_->Data();
    std::unique_ptr<TransformationMatrix> projection =
        TransformationMatrix::Create(m[0], m[1], m[2], m[3], m[4], m[5], m[6],
                                     m[7], m[8], m[9], m[10], m[11], m[12],
                                     m[13], m[14], m[15]);
    inv_projection_ = TransformationMatrix::Create(projection->Inverse());
    inv_projection_dirty_ = false;
  }

  // Transform the x/y coordinate into normalized screen space. Z coordinate of
  // -1 means the point will be projected onto the projection matrix near plane.
  FloatPoint3D point(x / width * 2.0 - 1.0, (height - y) / height * 2.0 - 1.0,
                     -1.0);

  FloatPoint3D unprojected_point = inv_projection_->MapPoint(point);

  // TODO(bajones): These should be constants.
  FloatPoint3D kOrigin(0.0, 0.0, 0.0);
  FloatPoint3D kUp(0.0, 1.0, 0.0);

  // Generate a "Look At" matrix
  FloatPoint3D z_axis = kOrigin - unprojected_point;
  z_axis.Normalize();

  FloatPoint3D x_axis = kUp.Cross(z_axis);
  x_axis.Normalize();

  FloatPoint3D y_axis = z_axis.Cross(x_axis);
  y_axis.Normalize();

  // TODO(bajones): There's probably a more efficent way to do this?
  TransformationMatrix inv_pointer(x_axis.X(), y_axis.X(), z_axis.X(), 0.0,
                                   x_axis.Y(), y_axis.Y(), z_axis.Y(), 0.0,
                                   x_axis.Z(), y_axis.Z(), z_axis.Z(), 0.0, 0.0,
                                   0.0, 0.0, 1.0);
  inv_pointer.Translate3d(-unprojected_point.X(), -unprojected_point.Y(),
                          -unprojected_point.Z());

  // LookAt matrices are view matrices (inverted), so invert before returning.
  std::unique_ptr<TransformationMatrix> pointer =
      TransformationMatrix::Create(inv_pointer.Inverse());

  return pointer;
}

DEFINE_TRACE(VRView) {
  visitor->Trace(frame_);
  visitor->Trace(projection_matrix_);
}

}  // namespace blink
