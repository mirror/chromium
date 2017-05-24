// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef VRCoordinateSystem_h
#define VRCoordinateSystem_h

#include "core/dom/DOMTypedArray.h"
#include "platform/bindings/ScriptWrappable.h"
#include "platform/heap/Handle.h"
#include "platform/wtf/Forward.h"

namespace blink {

class VRCoordinateSystem : public GarbageCollected<VRCoordinateSystem>,
                           public ScriptWrappable {
  DEFINE_WRAPPERTYPEINFO();

 public:
  VRCoordinateSystem() {}

  DOMFloat32Array* getTransformTo(VRCoordinateSystem* other) const;

  DECLARE_VIRTUAL_TRACE();
};

}  // namespace blink

#endif  // VRWebGLLayer_h
