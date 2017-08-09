// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "modules/vr/latest/VRCoordinateSystem.h"

#include "modules/vr/latest/VRSession.h"

namespace blink {

VRCoordinateSystem::VRCoordinateSystem(VRSession* session)
    : session_(session) {}

VRCoordinateSystem::~VRCoordinateSystem() {}

DOMFloat32Array* VRCoordinateSystem::getTransformTo(
    VRCoordinateSystem* other) const {
  if (session_ != other->session()) {
    // Cannot get relationships between coordinate systems that belong to
    // different sessions.
    return nullptr;
  }

  // TODO(bajones): Track relationship to other coordinate systems and return
  // the transforms here.
  return nullptr;
}

DEFINE_TRACE(VRCoordinateSystem) {
  visitor->Trace(session_);
}

}  // namespace blink
