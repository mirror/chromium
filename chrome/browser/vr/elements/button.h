// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_VR_ELEMENTS_BUTTON_H_
#define CHROME_BROWSER_VR_ELEMENTS_BUTTON_H_

#include <memory>

#include "base/callback.h"
#include "base/macros.h"
#include "chrome/browser/vr/elements/draw_phase.h"
#include "chrome/browser/vr/elements/ui_element.h"
#include "chrome/browser/vr/model/color_scheme.h"

namespace gfx {
class PointF;
}  // namespace gfx

namespace vr {

class Rect;

// Button has a circle as the background and a vector icon as the foreground.
// When hovered, background and foreground both move forward on Z axis.
// This matches the Daydream disk-style button.
class Button : public UiElement {
 public:
  explicit Button(base::Callback<void()> click_handler);
  ~Button() override;

  void Render(UiElementRenderer* renderer,
              const CameraModel& model) const final;

  Rect* background() const { return background_; }
  UiElement* hit_plane() const { return hit_plane_; }
  void SetButtonColors(const ButtonColors& colors);

 protected:
  bool hovered() const { return hovered_; }
  bool down() const { return down_; }
  bool pressed() const { return pressed_; }
  bool disabled() const { return disabled_; }
  const ButtonColors& colors() const { return colors_; }

  void OnSetDrawPhase() override;
  void OnSetName() override;
  void OnSetSize(const gfx::SizeF& size) override;
  void NotifyClientSizeAnimated(const gfx::SizeF& size,
                                int target_property_id,
                                cc::Animation* animation) override;
  virtual void OnStateUpdated();

 private:
  void HandleHoverEnter();
  void HandleHoverMove(const gfx::PointF& position);
  void HandleHoverLeave();
  void HandleButtonDown();
  void HandleButtonUp();

  bool down_ = false;
  bool hovered_ = false;
  bool pressed_ = false;
  bool disabled_ = false;
  base::Callback<void()> click_handler_;
  ButtonColors colors_;

  Rect* background_;
  UiElement* hit_plane_;

  DISALLOW_COPY_AND_ASSIGN(Button);
};

}  // namespace vr

#endif  // CHROME_BROWSER_VR_ELEMENTS_BUTTON_H_
