// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_VR_ELEMENTS_STEREO_BACKGROUND_H_
#define CHROME_BROWSER_VR_ELEMENTS_STEREO_BACKGROUND_H_

#include "base/callback.h"
#include "chrome/browser/vr/elements/ui_element.h"

#include "ui/gl/gl_bindings.h"

namespace vr {

class StereoBackground : public UiElement {
 public:
  StereoBackground();
  ~StereoBackground() override;
  void Initialize() override;
  void Render(UiElementRenderer* renderer,
              gfx::Transform view_proj_matrix,
              bool right_eye) const override;

 private:
  GLuint texture_handle_;

  DISALLOW_COPY_AND_ASSIGN(StereoBackground);
};

}  // namespace vr

#endif  // CHROME_BROWSER_VR_ELEMENTS_STEREO_BACKGROUND_H_
