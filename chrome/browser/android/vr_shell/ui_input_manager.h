// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_ANDROID_VR_SHELL_UI_INPUT_MANAGER_H_
#define CHROME_BROWSER_ANDROID_VR_SHELL_UI_INPUT_MANAGER_H_

#include <memory>
#include <queue>
#include <utility>
#include <vector>

#include "base/cancelable_callback.h"
#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "base/memory/weak_ptr.h"
#include "base/optional.h"
#include "base/single_thread_task_runner.h"
#include "chrome/browser/android/vr_shell/ui_scene.h"
#include "chrome/browser/android/vr_shell/vr_controller.h"
#include "chrome/browser/android/vr_shell/vr_controller_model.h"
#include "ui/gfx/native_widget_types.h"

namespace vr_shell {

class UiInputManagerDelegate {
 public:
  virtual ~UiInputManagerDelegate();

  virtual void OnEnterWebContents(const gfx::PointF& normalized_hit_point) = 0;
  virtual void OnExitWebContents() = 0;
  virtual void OnMoveOnWebContents(const gfx::PointF& normalized_hit_point) = 0;
  virtual void OnDownOnWebContents(const gfx::PointF& normalized_hit_point) = 0;
  virtual void OnUpOnWebContents(const gfx::PointF& normalized_hit_point) = 0;
};

class UiInputManager {
 public:
  enum ButtonState { UP, DOWN, CLICK, UNDEFINED };
  UiInputManager(UiScene* scene, UiInputManagerDelegate* delegate);
  ~UiInputManager();
  void UpdateController(const gfx::Vector3dF& controller_direction,
                        const gfx::Point3F& pointer_start,
                        ButtonState button_state);

  // TODO(tiborg): find better solution to pass this information.
  gfx::Point3F GetTargetPoint();
  UiElement* GetReticleRenderTarget();

 private:
  void SendHoverLeave(UiElement* target);
  bool SendHoverEnter(UiElement* target, const gfx::PointF& target_point);
  void SendHoverMove(const gfx::PointF& target_point);
  void SendButtonDown(UiElement* target,
                      const gfx::PointF& target_point,
                      ButtonState button_state);
  void SendButtonUp(UiElement* target,
                    const gfx::PointF& target_point,
                    ButtonState button_state);
  void GetVisualTargetElement(const gfx::Vector3dF& controller_direction,
                              gfx::Vector3dF& eye_to_target,
                              gfx::Point3F& target_point,
                              UiElement** target_element,
                              gfx::PointF& target_local_point) const;
  bool GetTargetLocalPoint(const gfx::Vector3dF& eye_to_target,
                           const UiElement& element,
                           float max_distance_to_plane,
                           gfx::PointF& target_local_point,
                           gfx::Point3F& target_point,
                           float& distance_to_plane) const;

  UiScene* const scene_;
  UiInputManagerDelegate* const delegate_;

  UiElement* hover_target_ = nullptr;
  UiElement* input_locked_element_ = nullptr;
  UiElement* reticle_render_target_ = nullptr;
  bool in_click_ = false;
  bool in_scroll_ = false;
  gfx::Point3F target_point_;
  gfx::Point3F pointer_start_;

  ButtonState previous_button_state_ = ButtonState::UNDEFINED;
};

}  // namespace vr_shell

#endif  // CHROME_BROWSER_ANDROID_VR_SHELL_UI_INPUT_MANAGER_H_
