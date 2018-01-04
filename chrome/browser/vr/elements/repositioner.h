// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_VR_ELEMENTS_REPOSITIONER_H_
#define CHROME_BROWSER_VR_ELEMENTS_REPOSITIONER_H_

#include <sstream>

#include "base/macros.h"
#include "chrome/browser/vr/elements/ui_element.h"
#include "chrome/browser/vr/model/reticle_model.h"
#include "ui/gfx/transform.h"

namespace vr {

// A Scaler adjusts the depth of its descendents by applying a scale. This
// permits dimensions in the subtree to be expressed in DM directly. Its main
// contribution is a tailored local transform that accounts for adjustments made
// by other Repositioner elements on its ancestor chain.
class Repositioner : public UiElement {
 public:
  explicit Repositioner(float content_depth);
  ~Repositioner() override;

  void set_laser_origin(const gfx::Point3F& laser_origin) {
    laser_origin_ = laser_origin;
  }

  void set_laser_direction(const gfx::Vector3dF& laser_direction) {
    laser_direction_ = laser_direction;
  }

  void set_enable(bool enable) { enabled_ = enable; }

  void SetTransform();

 private:
  gfx::Transform LocalTransform() const override;
  gfx::Transform GetTargetLocalTransform() const override;
  bool OnBeginFrame(const base::TimeTicks& time,
                    const gfx::Vector3dF& look_at) override;
  void DumpGeometry(std::ostringstream* os) const override;

  gfx::Transform transform_;
  bool enabled_ = false;
  float content_depth_;
  gfx::Point3F laser_origin_;
  gfx::Vector3dF laser_direction_;

  DISALLOW_COPY_AND_ASSIGN(Repositioner);
};

}  // namespace vr

#endif  // CHROME_BROWSER_VR_ELEMENTS_REPOSITIONER_H_
