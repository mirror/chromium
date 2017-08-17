// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_VR_ELEMENTS_VIEWPORT_AWARE_ROOT_H_
#define CHROME_BROWSER_VR_ELEMENTS_VIEWPORT_AWARE_ROOT_H_

#include "base/macros.h"
#include "chrome/browser/vr/elements/ui_element.h"
#include "ui/gfx/transform.h"

namespace vr {

class ViewportAwareRoot : public UiElement {
 public:
  static const float kViewportRotationTriggerDegrees;

  ViewportAwareRoot();
  ~ViewportAwareRoot() override;

  void AdjustPosition(const gfx::Vector3dF& head_pose) override;
  void LayOutChildren() override;

  gfx::Transform LocalTransform() const override;

 private:
  gfx::Transform viewport_aware_rotation_;

  DISALLOW_COPY_AND_ASSIGN(ViewportAwareRoot);
};

}  // namespace vr

#endif  // CHROME_BROWSER_VR_ELEMENTS_VIEWPORT_AWARE_ROOT_H_
