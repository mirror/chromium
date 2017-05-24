// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef VRDevicePose_h
#define VRDevicePose_h

#include "core/dom/DOMTypedArray.h"
#include "platform/bindings/ScriptWrappable.h"
#include "platform/heap/Handle.h"
#include "platform/wtf/Forward.h"

namespace blink {

class VRView;

class VRDevicePose final : public GarbageCollected<VRDevicePose>,
                           public ScriptWrappable {
  DEFINE_WRAPPERTYPEINFO();

 public:
  VRDevicePose(DOMFloat32Array* pose_model_matrix);

  DOMFloat32Array* poseModelMatrix() const { return pose_model_matrix_; }

  DOMFloat32Array* getViewMatrix(VRView*);

  DECLARE_VIRTUAL_TRACE();

 private:
  Member<DOMFloat32Array> pose_model_matrix_;
};

}  // namespace blink

#endif  // VRWebGLLayer_h
