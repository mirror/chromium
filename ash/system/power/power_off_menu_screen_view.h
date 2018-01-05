// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASH_SYSTEM_POWER_POWER_OFF_MENU_SCREEN_VIEW_H_
#define ASH_SYSTEM_POWER_POWER_OFF_MENU_SCREEN_VIEW_H_

#include "ash/ash_export.h"
#include "ui/display/display_observer.h"
#include "ui/views/view.h"

namespace ash {

class PowerOffMenuView;

class ASH_EXPORT PowerOffMenuScreenView : public views::View,
                                          public display::DisplayObserver {
 public:
  PowerOffMenuScreenView();
  ~PowerOffMenuScreenView() override;

  PowerOffMenuView* power_off_menu_view() { return power_off_menu_view_; }

  // Schedules an animation to show or hide the view.
  void ScheduleShowHideAnimation(bool show);

 private:
  class PowerOffMenuBackgroundView;

  // views::View:
  void Layout() override;

  // ui::EventHandler:
  void OnGestureEvent(ui::GestureEvent* event) override;

  // display::DisplayObserver:
  void OnDisplayMetricsChanged(const display::Display& display,
                               uint32_t changed_metrics) override;

  // Created by PowerOffMenuScreenView. Owned by views hierarchy.
  PowerOffMenuView* power_off_menu_view_ = nullptr;
  PowerOffMenuBackgroundView* power_off_screen_background_shield_ = nullptr;

  DISALLOW_COPY_AND_ASSIGN(PowerOffMenuScreenView);
};

}  // namespace ash

#endif
