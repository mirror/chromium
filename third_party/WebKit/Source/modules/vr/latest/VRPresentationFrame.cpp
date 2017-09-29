// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "modules/vr/latest/VRPresentationFrame.h"

#include "modules/vr/latest/VRCoordinateSystem.h"
#include "modules/vr/latest/VRDevicePose.h"
#include "modules/vr/latest/VRSession.h"
#include "modules/vr/latest/input/VRInputPose.h"
#include "modules/vr/latest/input/VRInputSource.h"

namespace blink {

VRPresentationFrame::VRPresentationFrame(VRSession* session)
    : session_(session) {
  // Add the left and right views to the presentation frame
  views_.push_back(new VRView(this, VRView::kEyeLeft));

  // TODO(bajones): This should be a bit more dynamic.
  if (session->exclusive()) {
    views_.push_back(new VRView(this, VRView::kEyeRight));
  }
}

VRDevicePose* VRPresentationFrame::getDevicePose(
    VRCoordinateSystem* coordinate_system) const {
  // If we don't have a valid base pose return null. Most common when tracking
  // is lost.
  if (!base_pose_matrix_ || !coordinate_system) {
    return nullptr;
  }

  // Must use a coordinate system created from the same session.
  if (coordinate_system->session() != session_) {
    return nullptr;
  }

  std::unique_ptr<TransformationMatrix> pose =
      coordinate_system->TransformBasePose(*base_pose_matrix_);

  if (!pose) {
    return nullptr;
  }

  return new VRDevicePose(session(), std::move(pose));
}

VRInputPose* VRPresentationFrame::getInputPose(
    VRInputSource* input_source,
    VRCoordinateSystem* coordinate_system) const {
  if (!input_source || !coordinate_system) {
    return nullptr;
  }

  // Must use an input source and coordinate system from the same session.
  if (input_source->session() != session_ ||
      coordinate_system->session() != session_) {
    return nullptr;
  }

  if (input_source->inputType() == kVRPointerInputSource) {
    // If the input source is a pointer event the base pose refers

    // Need the head's base pose and the pointer transform matrix.
    if (!base_pose_matrix_ || !input_source->pointer_transform_matrix_) {
      return nullptr;
    }

    // Multiply the head pose by the pointer transform to get the final pointer.
    std::unique_ptr<TransformationMatrix> pointer_pose =
        coordinate_system->TransformBasePose(*base_pose_matrix_);
    pointer_pose->Multiply(*(input_source->pointer_transform_matrix_));

    return new VRInputPose(nullptr, std::move(pointer_pose));
  }

  if (input_source->gaze_cursor()) {
    // If the input source uses a gaze cursor, return a pointer based on the
    // device pose.

    // If we don't have a valid base pose return null. Most common when tracking
    // is lost.
    if (!base_pose_matrix_) {
      return nullptr;
    }

    std::unique_ptr<TransformationMatrix> pointer_pose =
        coordinate_system->TransformBasePose(*base_pose_matrix_);

    return new VRInputPose(nullptr, std::move(pointer_pose));
  }

  // If the input source doesn't have a base pose return null;
  if (!input_source->base_pose_matrix_) {
    return nullptr;
  }

  std::unique_ptr<TransformationMatrix> grip_pose =
      coordinate_system->TransformBasePose(*(input_source->base_pose_matrix_));

  // TODO(bajones): This is currently wrong for "headModel" frame of
  // references, since the position of the controller will be stripped out.

  if (!grip_pose) {
    return nullptr;
  }

  std::unique_ptr<TransformationMatrix> pointer_pose(
      TransformationMatrix::Create(*grip_pose));

  if (input_source->pointer_transform_matrix_) {
    pointer_pose->Multiply(*(input_source->pointer_transform_matrix_));
  }

  return new VRInputPose(std::move(grip_pose), std::move(pointer_pose));
}

void VRPresentationFrame::UpdateBasePose(
    std::unique_ptr<TransformationMatrix> base_pose_matrix) {
  base_pose_matrix_ = std::move(base_pose_matrix);
}

DEFINE_TRACE(VRPresentationFrame) {
  visitor->Trace(views_);
  visitor->Trace(empty_views_);
  visitor->Trace(session_);
}

DEFINE_TRACE_WRAPPERS(VRPresentationFrame) {
  visitor->TraceWrappers(session_);
}

}  // namespace blink
