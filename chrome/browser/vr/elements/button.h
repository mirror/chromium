// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_VR_ELEMENTS_BUTTON_H_
#define CHROME_BROWSER_VR_ELEMENTS_BUTTON_H_

#include <memory>

#include "base/callback.h"
#include "base/macros.h"
#include "chrome/browser/vr/elements/invisible_hit_target.h"
#include "chrome/browser/vr/elements/ui_element.h"
#include "ui/gfx/vector_icon_types.h"

namespace gfx {
class PointF;
}  // namespace gfx

namespace vr {

class Rect;
class VectorIcon;

// Button has rounded rect as background and a vector icon as the foregroud.
// When hovered, background and foregroup both moving forward on Z axis.
class Button : public InvisibleHitTarget {
 public:
  Button(base::Callback<void()> click_handler,
         int draw_phase,
         float width,
         float height,
         const gfx::VectorIcon& icon);
  ~Button() override;

  void OnHoverLeave() override;
  void OnHoverEnter(const gfx::PointF& position) override;
  void OnMove(const gfx::PointF& position) override;
  void OnButtonDown(const gfx::PointF& position) override;
  void OnButtonUp(const gfx::PointF& position) override;
  bool HitTest(const gfx::PointF& point) const override;

  Rect* background() const { return background_; }
  VectorIcon* foreground() const { return foreground_; }
  void SetBackgroundColor(SkColor color);
  void SetBackgroundColorHovered(SkColor color);
  void SetBackgroundColorPressed(SkColor color);
  void SetVectorIconColor(SkColor color);

 private:
  void OnStateUpdated(const gfx::PointF& position);

  bool down_ = false;

  bool hovered_ = false;
  bool pressed_ = false;
  base::Callback<void()> click_handler_;
  SkColor background_color_ = SK_ColorWHITE;
  SkColor background_color_hovered_ = SK_ColorWHITE;
  SkColor background_color_pressed_ = SK_ColorWHITE;
  SkColor foreground_color_ = SK_ColorWHITE;

  Rect* background_;
  VectorIcon* foreground_;

  DISALLOW_COPY_AND_ASSIGN(Button);
};

}  // namespace vr

#endif  // CHROME_BROWSER_VR_ELEMENTS_BUTTON_H_
