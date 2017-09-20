// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_VR_VR_GL_LINUX_H_
#define CHROME_BROWSER_VR_VR_GL_LINUX_H_

#include "base/macros.h"

#include <cstdint>

#include "chrome/browser/vr/content_input_delegate.h"
#include "chrome/browser/vr/ui_browser_interface.h"
#include "chrome/browser/vr/vr_gl.h"

namespace vr {

class UiSceneManager;

class VrGlLinux : public VrGl,
                  public vr::ContentInputDelegate,
                  public vr::UiBrowserInterface {
 public:
  // VrGlLinux(vr::UiScene* scene);
  VrGlLinux();
  ~VrGlLinux() override;

  void DrawFrame(int16_t frame_index) override;

  // vr::ContentInputDelegate.
  void OnContentEnter(const gfx::PointF& normalized_hit_point) override;
  void OnContentLeave() override;
  void OnContentMove(const gfx::PointF& normalized_hit_point) override;
  void OnContentDown(const gfx::PointF& normalized_hit_point) override;
  void OnContentUp(const gfx::PointF& normalized_hit_point) override;
  void OnContentFlingStart(std::unique_ptr<blink::WebGestureEvent> gesture,
                           const gfx::PointF& normalized_hit_point) override;
  void OnContentFlingCancel(std::unique_ptr<blink::WebGestureEvent> gesture,
                            const gfx::PointF& normalized_hit_point) override;
  void OnContentScrollBegin(std::unique_ptr<blink::WebGestureEvent> gesture,
                            const gfx::PointF& normalized_hit_point) override;
  void OnContentScrollUpdate(std::unique_ptr<blink::WebGestureEvent> gesture,
                             const gfx::PointF& normalized_hit_point) override;
  void OnContentScrollEnd(std::unique_ptr<blink::WebGestureEvent> gesture,
                          const gfx::PointF& normalized_hit_point) override;

  // vr::UiBrowserInterface implementation (UI calling to VrShell).
  void ExitPresent() override;
  void ExitFullscreen() override;
  void NavigateBack() override;
  void ExitCct() override;
  void OnUnsupportedMode(vr::UiUnsupportedMode mode) override;
  void OnExitVrPromptResult(vr::UiUnsupportedMode reason,
                            vr::ExitVrPromptChoice choice) override;
  void OnContentScreenBoundsChanged(const gfx::SizeF& bounds) override;

 protected:
  std::unique_ptr<vr::UiSceneManager> scene_manager_;

 private:
  DISALLOW_COPY_AND_ASSIGN(VrGlLinux);
};

}  // namespace vr

#endif  // CHROME_BROWSER_VR_VR_GL_LINUX_H_
