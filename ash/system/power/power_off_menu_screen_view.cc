// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/system/power/power_off_menu_screen_view.h"

#include "ash/system/power/power_off_menu_view.h"
#include "ui/compositor/layer.h"
#include "ui/compositor/layer_animation_observer.h"
#include "ui/compositor/scoped_layer_animation_settings.h"
#include "ui/display/display.h"
#include "ui/display/screen.h"
#include "ui/views/widget/widget.h"

namespace ash {

namespace {

// Color of the fullscreen background shield.
constexpr SkColor kShieldColor = SkColorSetRGB(0x00, 0x00, 0x00);

// Opacity of the power off menu fullscreen background shield.
constexpr float kPowerOffMenuOpacity = 0.6f;

}  // namespace

class PowerOffMenuScreenView::PowerOffMenuBackgroundView
    : public views::View,
      public ui::ImplicitAnimationObserver {
 public:
  PowerOffMenuBackgroundView() {
    SetPaintToLayer(ui::LAYER_SOLID_COLOR);
    layer()->SetColor(kShieldColor);
  }

  ~PowerOffMenuBackgroundView() override = default;

  void OnImplicitAnimationsCompleted() override {
    if (layer()->opacity() == 0.f) {
      SetVisible(false);
      GetWidget()->Hide();
    }
  }

  void ScheduleShowHideAnimation(bool show) {
    layer()->GetAnimator()->StopAnimating();
    layer()->SetOpacity(show ? 0.f : kPowerOffMenuOpacity);

    ui::ScopedLayerAnimationSettings animation(layer()->GetAnimator());
    animation.AddObserver(this);
    animation.SetTweenType(show ? gfx::Tween::EASE_IN_2
                                : gfx::Tween::FAST_OUT_LINEAR_IN);
    animation.SetTransitionDuration(base::TimeDelta::FromMilliseconds(
        PowerOffMenuView::kAnimationTimeoutMs));

    layer()->SetOpacity(show ? kPowerOffMenuOpacity : 0.f);
  }

 private:
  DISALLOW_COPY_AND_ASSIGN(PowerOffMenuBackgroundView);
};

PowerOffMenuScreenView::PowerOffMenuScreenView() {
  power_off_screen_background_shield_ = new PowerOffMenuBackgroundView();
  AddChildView(power_off_screen_background_shield_);

  power_off_menu_view_ = new PowerOffMenuView();
  AddChildView(power_off_menu_view_);

  display::Screen::GetScreen()->AddObserver(this);
}

PowerOffMenuScreenView::~PowerOffMenuScreenView() {
  display::Screen::GetScreen()->RemoveObserver(this);
}

void PowerOffMenuScreenView::ScheduleShowHideAnimation(bool show) {
  DCHECK(power_off_screen_background_shield_);
  DCHECK(power_off_menu_view_);

  power_off_screen_background_shield_->ScheduleShowHideAnimation(show);
  power_off_menu_view_->ScheduleShowHideAnimation(show);
}

void PowerOffMenuScreenView::Layout() {
  const gfx::Rect contents_bounds = GetContentsBounds();
  power_off_screen_background_shield_->SetBoundsRect(contents_bounds);

  // TODO(minch), gets the menu bounds according to the power button position.
  gfx::Rect menu_bounds = contents_bounds;
  menu_bounds.ClampToCenteredSize(power_off_menu_view_->GetPreferredSize());
  menu_bounds.set_y(menu_bounds.y() - PowerOffMenuView::kMenuViewTopPadding);
  power_off_menu_view_->SetBoundsRect(menu_bounds);
}

void PowerOffMenuScreenView::OnGestureEvent(ui::GestureEvent* event) {
  // Closes the menu if touch anywhere on the background shield.
  if (event->type() == ui::ET_GESTURE_TAP_DOWN)
    ScheduleShowHideAnimation(false);
}

void PowerOffMenuScreenView::OnDisplayMetricsChanged(
    const display::Display& display,
    uint32_t changed_metrics) {
  GetWidget()->SetBounds(
      display::Screen::GetScreen()->GetPrimaryDisplay().bounds());
}

}  // namespace ash
