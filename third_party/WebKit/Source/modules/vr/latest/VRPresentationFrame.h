// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef VRPresentationFrame_h
#define VRPresentationFrame_h

#include "modules/vr/latest/VRView.h"
#include "platform/bindings/ScriptWrappable.h"
#include "platform/heap/Handle.h"
#include "platform/wtf/Forward.h"

namespace blink {

class VRCoordinateSystem;
class VRDevicePose;

class VRPresentationFrame final : public GarbageCollected<VRPresentationFrame>,
                                  public ScriptWrappable {
  DEFINE_WRAPPERTYPEINFO();

 public:
  VRPresentationFrame();

  // HeapVector<VRView> views() const;
  VRDevicePose* getDevicePose(VRCoordinateSystem*) const;

  DECLARE_VIRTUAL_TRACE();

 private:
  // HeapVector<VRView> views_;
  Member<VRDevicePose> device_pose_;
};

}  // namespace blink

#endif  // VRWebGLLayer_h
