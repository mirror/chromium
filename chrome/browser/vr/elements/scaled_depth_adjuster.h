// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_VR_ELEMENTS_SCALED_DEPTH_ADJUSTER_H_
#define CHROME_BROWSER_VR_ELEMENTS_SCALED_DEPTH_ADJUSTER_H_

#include <sstream>

#include "base/macros.h"
#include "chrome/browser/vr/elements/ui_element.h"
#include "ui/gfx/transform.h"

namespace vr {

// A Scaler adjusts the depth of its descendents by applying a scale. This
// permits dimensions in the subtree to be expressed in DM directly.
class ScaledDepthAdjuster : public UiElement {
 public:
  explicit ScaledDepthAdjuster(float delta_z);
  ~ScaledDepthAdjuster() override;

  float delta_z() const { return delta_z_; }

 private:
  gfx::Transform LocalTransform() const override;
  gfx::Transform GetTargetLocalTransform() const override;
  bool OnBeginFrame(const base::TimeTicks& time,
                    const gfx::Vector3dF& look_at) override;
  void OnSetType() override;
  void DumpGeometry(std::ostringstream* os) const override;

  gfx::Transform transform_;
  float delta_z_;

  DISALLOW_COPY_AND_ASSIGN(ScaledDepthAdjuster);
};

}  // namespace vr

#endif  // CHROME_BROWSER_VR_ELEMENTS_SCALED_DEPTH_ADJUSTER_H_
