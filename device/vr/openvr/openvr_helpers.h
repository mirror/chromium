// Copyright (c) 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <math.h>
#include <vector>

#include "device/vr/vr_service.mojom.h"
#include "third_party/openvr/src/headers/openvr.h"

namespace device {
mojom::VRPosePtr OpenVRPoseToVRPosePtr(const vr::TrackedDevicePose_t& hmd_pose);
}