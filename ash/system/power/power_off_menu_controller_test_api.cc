// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/system/power/power_off_menu_controller_test_api.h"

#include "ash/system/power/power_off_menu_controller.h"
#include "ash/system/power/power_off_menu_screen_view.h"
#include "ash/system/power/power_off_menu_view.h"
#include "ui/gfx/geometry/rect.h"
#include "ui/views/widget/widget.h"

namespace ash {

PowerOffMenuControllerTestApi::PowerOffMenuControllerTestApi(
    PowerOffMenuController* controller)
    : controller_(controller) {}

PowerOffMenuControllerTestApi::~PowerOffMenuControllerTestApi() = default;

bool PowerOffMenuControllerTestApi::PowerOffMenuTimerIsRunning() const {
  return controller_->power_off_menu_timer_.IsRunning();
}

bool PowerOffMenuControllerTestApi::TriggerPowerOffMenuTimeout() {
  if (!controller_->power_off_menu_timer_.IsRunning())
    return false;

  base::Closure task = controller_->power_off_menu_timer_.user_task();
  controller_->power_off_menu_timer_.Stop();
  task.Run();
  return true;
}

gfx::Rect PowerOffMenuControllerTestApi::GetMenuBoundsInScreen() const {
  return IsMenuOpened() ? GetPowerOffMenuView()->GetBoundsInScreen()
                        : gfx::Rect();
}

PowerOffMenuView* PowerOffMenuControllerTestApi::GetPowerOffMenuView() const {
  return IsMenuOpened()
             ? static_cast<PowerOffMenuScreenView*>(
                   controller_->fullscreen_widget_->GetContentsView())
                   ->power_off_menu_view()
             : nullptr;
}

bool PowerOffMenuControllerTestApi::IsMenuOpened() const {
  return controller_ && controller_->IsMenuOpened();
}

bool PowerOffMenuControllerTestApi::HasSignOut() const {
  return IsMenuOpened() && GetPowerOffMenuView()->has_sign_out();
}

}  // namespace ash
