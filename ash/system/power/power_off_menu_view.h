// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASH_SYSTEM_POWER_OFF_MENU_VIEW_H_
#define ASH_SYSTEM_POWER_OFF_MENU_VIEW_H_

#include "ash/ash_export.h"
#include "ui/compositor/layer_animation_observer.h"
#include "ui/views/controls/button/button.h"
#include "ui/views/view.h"

namespace ash {
class PowerOffMenuItemView;

class ASH_EXPORT PowerOffMenuView : public views::View,
                                    public views::ButtonListener,
                                    public ui::ImplicitAnimationObserver {
 public:
  // The duration of the animation to show or hide the power off menu view.
  static constexpr int kAnimationTimeoutMs = 500;

  // Top padding of the menu view.
  static constexpr int kMenuViewTopPadding = 16;

  PowerOffMenuView();
  ~PowerOffMenuView() override;

  // Schedules an animation to show or hide the view.
  void ScheduleShowHideAnimation(bool show);

  // Chehcks the |sign_out_item_| for tests.
  bool has_sign_out() { return sign_out_item_; }

 private:
  // Create the menu items that in the menu view.
  void CreateItems();

  // views::View
  void Layout() override;
  void OnPaint(gfx::Canvas* canvas) override;
  gfx::Size CalculatePreferredSize() const override;

  // views::ButtonListener
  void ButtonPressed(views::Button* sender, const ui::Event& event) override;

  // display::DisplayObserver:
  void OnImplicitAnimationsCompleted() override;

  // Items in the menu.
  PowerOffMenuItemView* power_off_item_ = nullptr;
  PowerOffMenuItemView* sign_out_item_ = nullptr;

  DISALLOW_COPY_AND_ASSIGN(PowerOffMenuView);
};

}  // namespace ash

#endif  // ASH_SYSTEM_POWER_OFF_MENU_VIEW_H_
