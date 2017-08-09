// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "modules/vr/latest/VRFrameOfReference.h"

#include "modules/vr/latest/VRDevicePose.h"
#include "modules/vr/latest/VRStageBounds.h"

namespace blink {

VRFrameOfReference::VRFrameOfReference(VRSession* session, Type type)
    : VRCoordinateSystem(session), type_(type) {}

VRFrameOfReference::~VRFrameOfReference() {}

void VRFrameOfReference::UpdatePoseTransform(
    std::unique_ptr<TransformationMatrix> transform) {
  pose_transform_ = std::move(transform);
}

void VRFrameOfReference::UpdateStageBounds(VRStageBounds* bounds) {
  bounds_ = bounds;
  // TODO(bajones): Fire a `boundschange` event
}

VRDevicePose* VRFrameOfReference::GetDevicePose(
    const TransformationMatrix& base_pose) {
  VRDevicePose* device_pose = nullptr;

  switch (type_) {
    case kTypeHeadModel: {
      // TODO(bajones): Detect if base pose is already neck modeled and return
      // it unchanged if so for better performance.

      // Strip out current position
      std::unique_ptr<TransformationMatrix> pose(
          TransformationMatrix::Create(base_pose));
      pose->SetM41(0.0);
      pose->SetM42(0.0);
      pose->SetM43(0.0);
      // TODO(bajones): Apply our own neck model
      device_pose = new VRDevicePose(session(), std::move(pose));
    } break;
    case kTypeEyeLevel:
      // For now we assume that all base poses are delivered as eye-level poses.
      // Thus in this case we just return the pose without transformation.
      device_pose =
          new VRDevicePose(session(), TransformationMatrix::Create(base_pose));
      break;
    case kTypeStage:
      // If the stage has a transform apply it to the base pose and return that,
      // otherwise return null.
      if (pose_transform_) {
        std::unique_ptr<TransformationMatrix> pose(
            TransformationMatrix::Create(base_pose));
        pose->Multiply(*pose_transform_);
        device_pose = new VRDevicePose(session(), std::move(pose));
      }
      break;
  }

  return device_pose;
}

DEFINE_TRACE(VRFrameOfReference) {
  visitor->Trace(bounds_);
  VRCoordinateSystem::Trace(visitor);
}

}  // namespace blink
