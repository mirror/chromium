// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_VR_VR_GL_H_
#define CHROME_BROWSER_VR_VR_GL_H_

#include <memory>

#include "base/macros.h"

namespace vr {
class ContentInputDelegate;
class UiBrowserInterface;
class UiScene;
class UiSceneManager;
class UiInputManager;
class UiRenderer;
struct UiInitialState;
}  // namespace vr

namespace vr {

class VrShellRenderer;

// This class manages all GLThread owned objects and GL rendering for VrShell.
// It is not threadsafe and must only be used on the GL thread.
class VrGl {
 public:
  VrGl(UiBrowserInterface* browser,
       ContentInputDelegate* content_input_delegate,
       const vr::UiInitialState& ui_initial_state);
  virtual ~VrGl();

 protected:
  bool ShouldDrawWebVr();

  virtual void DrawFrame(int16_t frame_index) = 0;

  void OnGlInitialized(unsigned int content_texture_id);

  std::unique_ptr<vr::UiScene> scene_;
  std::unique_ptr<vr::UiSceneManager> scene_manager_;
  std::unique_ptr<vr::VrShellRenderer> vr_shell_renderer_;
  std::unique_ptr<vr::UiInputManager> input_manager_;
  std::unique_ptr<vr::UiRenderer> ui_renderer_;

  bool web_vr_mode_;

 private:
  DISALLOW_COPY_AND_ASSIGN(VrGl);
};

}  // namespace vr

#endif  // CHROME_BROWSER_VR_VR_GL_H_
