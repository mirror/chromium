// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "modules/vr/latest/input/VRInputPose.h"

namespace blink {

namespace {

DOMFloat32Array* transformationMatrixToFloat32Array(
    const TransformationMatrix& matrix) {
  float array[] = {
      static_cast<float>(matrix.M11()), static_cast<float>(matrix.M12()),
      static_cast<float>(matrix.M13()), static_cast<float>(matrix.M14()),
      static_cast<float>(matrix.M21()), static_cast<float>(matrix.M22()),
      static_cast<float>(matrix.M23()), static_cast<float>(matrix.M24()),
      static_cast<float>(matrix.M31()), static_cast<float>(matrix.M32()),
      static_cast<float>(matrix.M33()), static_cast<float>(matrix.M34()),
      static_cast<float>(matrix.M41()), static_cast<float>(matrix.M42()),
      static_cast<float>(matrix.M43()), static_cast<float>(matrix.M44())};

  return DOMFloat32Array::Create(array, 16);
}

}  // namespace

VRInputPose::VRInputPose(
    std::unique_ptr<TransformationMatrix> grip_pose_matrix,
    std::unique_ptr<TransformationMatrix> pointer_pose_matrix)
    : grip_pose_matrix_(std::move(grip_pose_matrix)),
      pointer_pose_matrix_(std::move(pointer_pose_matrix)) {}

VRInputPose::~VRInputPose() {}

DOMFloat32Array* VRInputPose::gripPoseMatrix() const {
  if (!grip_pose_matrix_)
    return nullptr;
  return transformationMatrixToFloat32Array(*grip_pose_matrix_);
}

DOMFloat32Array* VRInputPose::pointerPoseMatrix() const {
  if (!pointer_pose_matrix_)
    return nullptr;
  return transformationMatrixToFloat32Array(*pointer_pose_matrix_);
}

}  // namespace blink
