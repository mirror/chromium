// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/system/power/power_off_menu_view.h"

#include "ash/session/session_controller.h"
#include "ash/shell.h"
#include "ash/strings/grit/ash_strings.h"
#include "ash/system/power/power_off_menu_item_view.h"
#include "ash/system/user/login_status.h"
#include "ash/wm/lock_state_controller.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/compositor/scoped_layer_animation_settings.h"
#include "ui/gfx/canvas.h"

namespace ash {

namespace {
constexpr int kItemViewWidth = 96;
constexpr int kItemViewHeight = 96;
constexpr int kItemViewInset = 16;
}  // namespace

PowerOffMenuView::PowerOffMenuView() {
  SetPaintToLayer();
  layer()->SetFillsBoundsOpaquely(false);
  //  layer()->SetOpacity(1.0f);

  CreateItems();
}

PowerOffMenuView::~PowerOffMenuView() = default;

void PowerOffMenuView::ScheduleShowHideAnimation(bool show) {
  layer()->GetAnimator()->StopAnimating();

  // SetVisible(true);
  layer()->SetOpacity(show ? 0.f : 1.0f);

  ui::ScopedLayerAnimationSettings animation(layer()->GetAnimator());
  animation.AddObserver(this);
  animation.SetTweenType(show ? gfx::Tween::EASE_IN
                              : gfx::Tween::FAST_OUT_LINEAR_IN);
  animation.SetTransitionDuration(base::TimeDelta::FromMilliseconds(500));

  layer()->SetOpacity(show ? 1.0f : 0.f);

  // animation of the bounds change.
  if (show) {
    gfx::Transform transform;
    transform.Translate(0, 15);
    layer()->SetTransform(transform);
  } else {
    layer()->SetTransform(gfx::Transform());
  }
}

void PowerOffMenuView::CreateItems() {
  power_off_item_ = new PowerOffMenuItemView(
      this, kSystemPowerOffMenuPowerOffIcon,
      l10n_util::GetStringUTF16(IDS_ASH_POWER_OFF_MENU_POWER_OFF_BUTTON));
  AddChildView(power_off_item_);

  LoginStatus login_status = Shell::Get()->session_controller()->login_status();
  if (login_status != LoginStatus::NOT_LOGGED_IN) {
    sign_out_item_ = new PowerOffMenuItemView(
        this, kSystemPowerOffMenuSignOutIcon,
        user::GetLocalizedSignOutStringForStatus(login_status, false));
    AddChildView(sign_out_item_);
  }
}

void PowerOffMenuView::Layout() {
  gfx::Rect rect(GetContentsBounds());
  gfx::Rect power_off_rect(rect);
  power_off_rect.set_size(gfx::Size(kItemViewWidth, kItemViewHeight));
  power_off_rect.Offset(gfx::Vector2d(kItemViewInset, kItemViewInset));
  power_off_item_->SetBoundsRect(power_off_rect);

  if (sign_out_item_) {
    gfx::Rect sign_out_rect(rect);
    sign_out_rect.set_size(gfx::Size(kItemViewWidth, kItemViewHeight));
    sign_out_rect.Offset(
        gfx::Vector2d(kItemViewInset + kItemViewWidth, kItemViewInset));
    sign_out_item_->SetBoundsRect(sign_out_rect);
  }
}

void PowerOffMenuView::OnPaint(gfx::Canvas* canvas) {
  views::View::OnPaint(canvas);

  // Clip into a rounded rectangle.
  constexpr SkScalar radius = SkIntToScalar(16);
  constexpr SkScalar kRadius[8] = {radius, radius, radius, radius,
                                   radius, radius, radius, radius};
  SkPath path;
  path.addRoundRect(gfx::RectToSkRect(gfx::Rect(size())), kRadius);

  gfx::Rect rect(size());
  canvas->ClipPath(path, true);
  canvas->DrawColor(SK_ColorWHITE);
}

gfx::Size PowerOffMenuView::CalculatePreferredSize() const {
  gfx::Size menu_size;
  if (power_off_item_ && sign_out_item_) {
    menu_size = gfx::Size(224, 128);
  } else {
    // only power_off_item_
    menu_size = gfx::Size(128, 128);
  }
  return menu_size;
}

void PowerOffMenuView::ButtonPressed(views::Button* sender,
                                     const ui::Event& event) {
  DCHECK(sender);
  if (sender == power_off_item_) {
    Shell::Get()->lock_state_controller()->StartShutdownAnimation(
        ShutdownReason::POWER_BUTTON);
  } else if (sender == sign_out_item_) {
    Shell::Get()->session_controller()->RequestSignOut();
  }
}

void PowerOffMenuView::OnImplicitAnimationsCompleted() {
  if (layer()->opacity() == 0.f)
    SetVisible(false);
}

}  // namespace ash
