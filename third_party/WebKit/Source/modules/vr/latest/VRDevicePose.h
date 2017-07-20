// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef VRDevicePose_h
#define VRDevicePose_h

#include "core/typed_arrays/DOMTypedArray.h"
#include "platform/bindings/ScriptWrappable.h"
#include "platform/heap/Handle.h"
#include "platform/transforms/TransformationMatrix.h"
#include "platform/wtf/Forward.h"

namespace blink {

class VRSession;
class VRView;

class VRDevicePose final : public GarbageCollectedFinalized<VRDevicePose>,
                           public ScriptWrappable {
  DEFINE_WRAPPERTYPEINFO();

 public:
  VRDevicePose(VRSession*, std::unique_ptr<TransformationMatrix>);

  DOMFloat32Array* poseModelMatrix() const;
  DOMFloat32Array* getViewMatrix(VRView*);

  DECLARE_VIRTUAL_TRACE();

 private:
  const Member<VRSession> session_;
  std::unique_ptr<TransformationMatrix> pose_model_matrix_;
};

}  // namespace blink

#endif  // VRWebGLLayer_h
