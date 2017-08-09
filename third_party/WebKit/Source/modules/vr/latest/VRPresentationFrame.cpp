// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "modules/vr/latest/VRPresentationFrame.h"

#include "modules/vr/latest/VRCoordinateSystem.h"
#include "modules/vr/latest/VRDevicePose.h"
#include "modules/vr/latest/VRSession.h"

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

  return coordinate_system->GetDevicePose(*base_pose_matrix_);
}

void VRPresentationFrame::UpdateBasePose(
    std::unique_ptr<TransformationMatrix> base_pose_matrix) {
  base_pose_matrix_ = std::move(base_pose_matrix);
}

DEFINE_TRACE(VRPresentationFrame) {
  visitor->Trace(views_);
  visitor->Trace(session_);
}

DEFINE_TRACE_WRAPPERS(VRPresentationFrame) {
  visitor->TraceWrappers(session_);
}

}  // namespace blink
