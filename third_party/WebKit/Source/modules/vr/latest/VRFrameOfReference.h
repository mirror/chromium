// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef VRFrameOfReference_h
#define VRFrameOfReference_h

#include "modules/vr/latest/VRCoordinateSystem.h"
#include "platform/transforms/TransformationMatrix.h"

namespace blink {

class VRDevicePose;
class VRStageBounds;

class VRFrameOfReference final : public VRCoordinateSystem {
  DEFINE_WRAPPERTYPEINFO();

 public:
  enum Type { kTypeHeadModel, kTypeEyeLevel, kTypeStage };

  VRFrameOfReference(VRSession*, Type);
  ~VRFrameOfReference() override;

  void UpdatePoseTransform(std::unique_ptr<TransformationMatrix>);
  void UpdateStageBounds(VRStageBounds*);

  VRDevicePose* GetDevicePose(const TransformationMatrix& base_pose) override;

  VRStageBounds* bounds() const { return bounds_; }

  Type type() const { return type_; }

  DECLARE_VIRTUAL_TRACE();

 private:
  Member<VRStageBounds> bounds_;
  Type type_;
  std::unique_ptr<TransformationMatrix> pose_transform_;
};

}  // namespace blink

#endif  // VRWebGLLayer_h
