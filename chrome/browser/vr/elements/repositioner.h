// Copyright 2018 The Chromium Authors. All rights reserved.
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

// A repositioner adjusts the position of its children by rotation. The
// reposition is driven by controller.
class Repositioner : public UiElement {
 public:
  Repositioner();
  ~Repositioner() override;

  void set_laser_direction(const gfx::Vector3dF& laser_direction) {
    last_laser_direction_ = laser_direction_;
    laser_direction_ = laser_direction;
  }

  void SetEnabled(bool enabled);

 private:
  gfx::Transform LocalTransform() const override;
  gfx::Transform GetTargetLocalTransform() const override;
  void UpdateTransform(const gfx::Transform& head_pose);
  bool OnBeginFrame(const base::TimeTicks& time,
                    const gfx::Transform& head_pose) override;
#ifndef NDEBUG
  void DumpGeometry(std::ostringstream* os) const override;
#endif

  gfx::Transform transform_;
  bool enabled_ = false;
  gfx::Vector3dF laser_direction_;
  gfx::Vector3dF last_laser_direction_;
  bool snap_to_world_up_ = false;

  DISALLOW_COPY_AND_ASSIGN(Repositioner);
};

}  // namespace vr

#endif  // CHROME_BROWSER_VR_ELEMENTS_REPOSITIONER_H_
