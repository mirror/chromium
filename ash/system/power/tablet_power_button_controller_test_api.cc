// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/system/power/tablet_power_button_controller_test_api.h"

#include "ash/system/power/power_off_menu_screen_view.h"
#include "ash/system/power/power_off_menu_view.h"
#include "ash/system/power/tablet_power_button_controller.h"
#include "ui/gfx/geometry/rect.h"
#include "ui/views/widget/widget.h"

namespace ash {

TabletPowerButtonControllerTestApi::TabletPowerButtonControllerTestApi(
    TabletPowerButtonController* controller)
    : controller_(controller) {}

TabletPowerButtonControllerTestApi::~TabletPowerButtonControllerTestApi() =
    default;

bool TabletPowerButtonControllerTestApi::PowerOffMenuTimerIsRunning() const {
  return controller_->power_off_menu_timer_.IsRunning();
}

bool TabletPowerButtonControllerTestApi::TriggerPowerOffMenuTimeout() {
  if (!controller_->power_off_menu_timer_.IsRunning())
    return false;

  base::Closure task = controller_->power_off_menu_timer_.user_task();
  controller_->power_off_menu_timer_.Stop();
  task.Run();
  return true;
}

gfx::Rect TabletPowerButtonControllerTestApi::GetMenuBoundsInScreen() const {
  return IsMenuOpened() ? GetPowerOffMenuView()->GetBoundsInScreen()
                        : gfx::Rect();
}

PowerOffMenuView* TabletPowerButtonControllerTestApi::GetPowerOffMenuView()
    const {
  return IsMenuOpened()
             ? static_cast<PowerOffMenuScreenView*>(
                   controller_->fullscreen_widget_->GetContentsView())
                   ->power_off_menu_view()
             : nullptr;
}

bool TabletPowerButtonControllerTestApi::IsMenuOpened() const {
  return controller_ && controller_->IsMenuOpened();
}

bool TabletPowerButtonControllerTestApi::HasSignOut() const {
  return IsMenuOpened() && GetPowerOffMenuView()->has_sign_out();
}

}  // namespace ash
