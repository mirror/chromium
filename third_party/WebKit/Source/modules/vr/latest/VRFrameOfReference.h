// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef VRFrameOfReference_h
#define VRFrameOfReference_h

#include "modules/vr/latest/VRCoordinateSystem.h"

namespace blink {

class VRStageBounds;

class VRFrameOfReference final : public VRCoordinateSystem {
  DEFINE_WRAPPERTYPEINFO();

 public:
  VRFrameOfReference(VRStageBounds*);

  VRStageBounds* bounds() const { return bounds_; }

  DECLARE_VIRTUAL_TRACE();

 private:
  Member<VRStageBounds> bounds_;
};

}  // namespace blink

#endif  // VRWebGLLayer_h
