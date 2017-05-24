// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "modules/vr/latest/VRCoordinateSystem.h"

#include "core/dom/DOMTypedArray.h"

namespace blink {

DOMFloat32Array* VRCoordinateSystem::getTransformTo(
    VRCoordinateSystem* other) const {
  // Track relationship to other coordinate systems and return the transforms
  // here
  return nullptr;
}

DEFINE_TRACE(VRCoordinateSystem) {
  // visitor->Trace(vr_);
}

}  // namespace blink
