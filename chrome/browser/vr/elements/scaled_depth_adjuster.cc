// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/vr/elements/scaled_depth_adjuster.h"

namespace vr {

ScaledDepthAdjuster::ScaledDepthAdjuster(float delta_z) : delta_z_(delta_z) {
  set_type(kTypeScaledDepthAdjuster);
}

ScaledDepthAdjuster::~ScaledDepthAdjuster() = default;

gfx::Transform ScaledDepthAdjuster::LocalTransform() const {
  return transform_;
}

bool ScaledDepthAdjuster::OnBeginFrame(const base::TimeTicks& time,
                                       const gfx::Vector3dF& look_at) {
  if (!transform_.IsIdentity())
    return false;

  gfx::Transform inherited;
  for (UiElement* anc = parent(); anc; anc = anc->parent()) {
    if (anc->type() == kTypeScaledDepthAdjuster) {
      inherited.ConcatTransform(anc->LocalTransform());
    }
  }
  bool success = inherited.GetInverse(&transform_);
  CHECK(success);
  gfx::Point3F o;
  inherited.TransformPoint(&o);
  float z = -o.z() + delta_z_;
  transform_.Scale3d(z, z, z);
  transform_.Translate3d(0, 0, -1);
  return true;
}

void ScaledDepthAdjuster::OnSetType() {
  DCHECK_EQ(kTypeScaledDepthAdjuster, type());
}

void ScaledDepthAdjuster::DumpGeometry(std::ostringstream* os) const {
  gfx::Transform t = world_space_transform();
  gfx::Point3F o;
  t.TransformPoint(&o);
  *os << "tz(" << delta_z_ << ") origin =" << o.ToString();
}

}  // namespace vr
