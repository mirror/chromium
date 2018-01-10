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

  void set_reticle_position(const gfx::Point3F& targe_point) {
    target_point_ = targe_point;
  }

  void set_hit_test_request(const HitTestRequest& request) {
    hit_test_origin_ = request.ray_origin;
    hit_test_direction_ = request.ray_target - request.ray_origin;
  }

  void set_enable(bool enable) { enabled_ = enable; }

 private:
  gfx::Transform LocalTransform() const override;
  gfx::Transform GetTargetLocalTransform() const override;
  void UpdateTransform(const gfx::Transform& head_pose);
  bool OnBeginFrame(const base::TimeTicks& time,
                    const gfx::Transform& head_pose) override;
  void DumpGeometry(std::ostringstream* os) const override;

  gfx::Transform transform_;
  bool enabled_ = false;
  gfx::Point3F hit_test_origin_;
  gfx::Vector3dF hit_test_direction_;

  bool dragging_ = false;
  gfx::Point3F target_point_;
  gfx::Vector3dF initial_reticle_vector_;

  DISALLOW_COPY_AND_ASSIGN(Repositioner);
};

}  // namespace vr

#endif  // CHROME_BROWSER_VR_ELEMENTS_REPOSITIONER_H_
