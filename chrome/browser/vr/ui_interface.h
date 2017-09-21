// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_VR_UI_INTERFACE_H_
#define CHROME_BROWSER_VR_UI_INTERFACE_H_

#include "chrome/browser/vr/browser_ui_interface.h"

namespace gfx {
class Transform;
}

namespace vr {

// This is the interface to the UI (from GL or the browser).
class UiInterface : public BrowserUiInterface {
 public:
  ~UiInterface() override {}

  virtual void OnGlInitialized(unsigned int content_texture_id) = 0;
  virtual void OnAppButtonClicked() = 0;
  virtual void OnAppButtonGesturePerformed(
      UiInterface::Direction direction) = 0;
  virtual void OnProjMatrixChanged(const gfx::Transform& proj_matrix) = 0;
  virtual void OnWebVrFrameAvailable() = 0;
  virtual void OnWebVrTimedOut() = 0;
};

}  // namespace vr

#endif  // CHROME_BROWSER_VR_UI_INTERFACE_H_
