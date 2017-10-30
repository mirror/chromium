// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_VR_ELEMENTS_LASER_H_
#define CHROME_BROWSER_VR_ELEMENTS_LASER_H_

#include "chrome/browser/vr/elements/ui_element.h"
#include "ui/gfx/geometry/point3_f.h"

namespace vr {

class Laser : public UiElement {
 public:
  Laser();
  ~Laser() override;

  // THIS SHOULD NOT BE BOUND BUT USED DIRECTLY FROM MODEL IN RENDER.
  void set_origin(const gfx::Point3F& origin) { origin_ = origin; }

  void set_target(const gfx::Point3F& target) { target_ = target; }

 private:
  void Render(UiElementRenderer* renderer,
              const gfx::Transform& model_view_proj_matrix) const final;

  gfx::Point3F origin_;
  gfx::Point3F target_;

  DISALLOW_COPY_AND_ASSIGN(Laser);
};

}  // namespace vr

#endif  // CHROME_BROWSER_VR_ELEMENTS_LASER_H_
