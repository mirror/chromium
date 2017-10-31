// Copyright (c) 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "device/vr/openvr/openvr_helpers.h"

#include <math.h>
#include <vector>

#include "device/vr/vr_service.mojom.h"
#include "third_party/openvr/src/headers/openvr.h"

namespace device {

namespace {
inline std::vector<float> HmdVector3ToWebVR(const vr::HmdVector3_t& vec) {
  std::vector<float> out;
  out.resize(3);
  out[0] = vec.v[0];
  out[1] = vec.v[1];
  out[2] = vec.v[2];
  return out;
}
}  // namespace

mojom::VRPosePtr OpenVRPoseToVRPosePtr(
    const vr::TrackedDevicePose_t& hmd_pose) {
  mojom::VRPosePtr pose = mojom::VRPose::New();
  pose->orientation.emplace(4);

  pose->orientation.value()[0] = 0;
  pose->orientation.value()[1] = 0;
  pose->orientation.value()[2] = 0;
  pose->orientation.value()[3] = 1;

  pose->position.emplace(3);
  pose->position.value()[0] = 0;
  pose->position.value()[1] = 0;
  pose->position.value()[2] = 0;

  if (hmd_pose.bPoseIsValid && hmd_pose.bDeviceIsConnected) {
    const float(&m)[3][4] = hmd_pose.mDeviceToAbsoluteTracking.m;
    float w = sqrt(1 + m[0][0] + m[1][1] + m[2][2]) / 2;
    pose->orientation.value()[0] = (m[2][1] - m[1][2]) / (4 * w);
    pose->orientation.value()[1] = (m[0][2] - m[2][0]) / (4 * w);
    pose->orientation.value()[2] = (m[1][0] - m[0][1]) / (4 * w);
    pose->orientation.value()[3] = w;

    pose->position.value()[0] = m[0][3];
    pose->position.value()[1] = m[1][3];
    pose->position.value()[2] = m[2][3];

    pose->linearVelocity = HmdVector3ToWebVR(hmd_pose.vVelocity);
    pose->angularVelocity = HmdVector3ToWebVR(hmd_pose.vAngularVelocity);
  }

  return pose;
}

}  // namespace device