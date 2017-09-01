// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "modules/vr/latest/input/VRControllerInputSource.h"

namespace blink {

VRControllerInputSource::VRControllerInputSource(
    VRSession* session,
    uint32_t index,
    bool gaze_cursor,
    const HeapHashMap<String, Member<VRControllerElement>>& elements)
    : VRInputSource(session, index, kVRControllerInputSource, gaze_cursor),
      elements_(new VRControllerElementMap(elements)) {
  setHandedness(handedness_);
}

VRControllerInputSource::~VRControllerInputSource() {}

void VRControllerInputSource::setHandedness(Handedness handedness) {
  handedness_ = handedness;
  switch (handedness_) {
    case kHandNone:
      handedness_string_ = "";
    case kHandLeft:
      handedness_string_ = "left";
    case kHandRight:
      handedness_string_ = "right";
  }
}

DEFINE_TRACE(VRControllerInputSource) {
  visitor->Trace(elements_);
  VRInputSource::Trace(visitor);
}

}  // namespace blink
