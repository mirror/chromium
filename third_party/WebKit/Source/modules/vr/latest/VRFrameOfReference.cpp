// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "modules/vr/latest/VRFrameOfReference.h"

#include "modules/vr/latest/VRStageBounds.h"

namespace blink {

VRFrameOfReference::VRFrameOfReference(VRStageBounds* bounds)
    : bounds_(bounds) {}

DEFINE_TRACE(VRFrameOfReference) {
  visitor->Trace(bounds_);
  VRCoordinateSystem::Trace(visitor);
}

}  // namespace blink
