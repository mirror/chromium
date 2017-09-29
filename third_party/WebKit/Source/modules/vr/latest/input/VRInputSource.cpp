// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "modules/vr/latest/input/VRInputSource.h"
#include "modules/vr/latest/VRSession.h"

namespace blink {

VRInputSource::VRInputSource(VRSession* session,
                             uint32_t index,
                             VRInputSourceType input_type,
                             bool gaze_cursor)
    : session_(session),
      index_(index),
      input_type_(input_type),
      gaze_cursor_(gaze_cursor) {}

VRInputSource::~VRInputSource() {}

void VRInputSource::set_base_pose_matrix(
    std::unique_ptr<TransformationMatrix> base_pose_matrix) {
  base_pose_matrix_ = std::move(base_pose_matrix);
}

void VRInputSource::set_pointer_transform_matrix(
    std::unique_ptr<TransformationMatrix> pointer_transform_matrix) {
  pointer_transform_matrix_ = std::move(pointer_transform_matrix);
}

DEFINE_TRACE(VRInputSource) {
  visitor->Trace(session_);
}

}  // namespace blink
