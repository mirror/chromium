// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "modules/vr/latest/VRPresentationFrame.h"

#include "modules/vr/latest/VRDevicePose.h"

namespace blink {

VRPresentationFrame::VRPresentationFrame() {}

/*HeapVector<VRView> VRPresentationFrame::views() const {
  // TODO(bajones): Mechanism for updating views
  return views_;
}*/

VRDevicePose* VRPresentationFrame::getDevicePose(
    VRCoordinateSystem* coordinate_system) const {
  // TODO(bajones): Mechanism for updating pose
  return device_pose_;
}

DEFINE_TRACE(VRPresentationFrame) {
  // visitor->Trace(views_);
  visitor->Trace(device_pose_);
}

}  // namespace blink
