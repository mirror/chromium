// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef VRInputPose_h
#define VRInputPose_h

#include "core/typed_arrays/DOMTypedArray.h"
#include "platform/bindings/ScriptWrappable.h"
#include "platform/heap/Handle.h"
#include "platform/transforms/TransformationMatrix.h"
#include "platform/wtf/Forward.h"

namespace blink {

class VRInputPose final : public GarbageCollectedFinalized<VRInputPose>,
                          public ScriptWrappable {
  DEFINE_WRAPPERTYPEINFO();

 public:
  VRInputPose(std::unique_ptr<TransformationMatrix> grip_pose_matrix,
              std::unique_ptr<TransformationMatrix> pointer_pose_matrix);
  ~VRInputPose();

  DOMFloat32Array* gripPoseMatrix() const;
  DOMFloat32Array* pointerPoseMatrix() const;

  DEFINE_INLINE_TRACE(){};

 private:
  const std::unique_ptr<TransformationMatrix> grip_pose_matrix_;
  const std::unique_ptr<TransformationMatrix> pointer_pose_matrix_;
};

}  // namespace blink

#endif  // VRLayer_h
