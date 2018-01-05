// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/system/power/power_off_menu_controller.h"

#include "ash/public/cpp/shell_window_ids.h"
#include "ash/shell.h"
#include "ash/system/power/power_button_controller_util.h"
#include "ash/system/power/power_button_display_controller.h"
#include "ash/system/power/power_off_menu_screen_view.h"
#include "base/time/tick_clock.h"
#include "ui/views/widget/widget.h"

namespace ash {

TabletPowerButtonController::TabletPowerButtonController(
    PowerButtonDisplayController* display_controller,
    base::TickClock* tick_clock)
    : display_controller_(display_controller), tick_clock_(tick_clock) {}

TabletPowerButtonController::~TabletPowerButtonController() = default;

void TabletPowerButtonController::OnPowerButtonEvent(
    bool down,
    const base::TimeTicks& timestamp) {
  if (down) {
    force_off_on_button_up_ = true;

    // When the system resumes in response to the power button being pressed,
    // Chrome receives powerd's SuspendDone signal and notification that the
    // backlight has been turned back on before seeing the power button events
    // that woke the system. Avoid forcing off display just after resuming to
    // ensure that we don't turn the display off in response to the events.
    if (timestamp - last_resume_time_ <=
        PowerButtonControllerUtil::kIgnorePowerButtonAfterResumeDelay)
      force_off_on_button_up_ = false;

    // The actual display may remain off for a short period after powerd asks
    // Chrome to turn it on. If the user presses the power button again during
    // this time, they probably intend to turn the display on. Avoid forcing off
    // in this case.
    if (timestamp - display_controller_->screen_state_last_changed() <=
        PowerButtonControllerUtil::kScreenStateChangeDelay) {
      force_off_on_button_up_ = false;
    }

    screen_off_when_power_button_down_ = !display_controller_->IsScreenOn();
    display_controller_->SetBacklightsForcedOff(false);
    power_off_menu_timer_.Start(
        FROM_HERE, kStartPowerOffMenuAnimationTimeout, this,
        &TabletPowerButtonController::OnPowerOffMenuTimeout);
  } else {
    const base::TimeTicks previous_up_time = last_button_up_time_;
    last_button_up_time_ = timestamp;

    // Ignore the event if it comes too soom after the last one.
    if (timestamp - previous_up_time <=
        PowerButtonControllerUtil::kIgnoreRepeatedButtonUpDelay) {
      power_off_menu_timer_.Stop();
      return;
    }

    if (power_off_menu_timer_.IsRunning()) {
      power_off_menu_timer_.Stop();
      if (!screen_off_when_power_button_down_ && force_off_on_button_up_) {
        display_controller_->SetBacklightsForcedOff(true);
        DismissMenu();
        PowerButtonControllerUtil::LockScreenIfRequired();
      }
    }
  }
}

bool TabletPowerButtonController::IsMenuOpened() const {
  return fullscreen_widget_ &&
         fullscreen_widget_->GetLayer()->GetTargetVisibility();
}

void TabletPowerButtonController::DismissMenu() {
  if (IsMenuOpened())
    fullscreen_widget_->Hide();
}

void TabletPowerButtonController::CancelTabletPowerButton() {
  force_off_on_button_up_ = false;
  power_off_menu_timer_.Stop();
}

void TabletPowerButtonController::OnPowerOffMenuTimeout() {
  fullscreen_widget_ = CreateFullscreenWidget();
  fullscreen_widget_->SetContentsView(new PowerOffMenuScreenView());

  static_cast<PowerOffMenuScreenView*>(fullscreen_widget_->GetContentsView())
      ->ScheduleShowHideAnimation(true);
}

std::unique_ptr<views::Widget>
TabletPowerButtonController::CreateFullscreenWidget() {
  std::unique_ptr<views::Widget> fullscreen_widget(new views::Widget);
  views::Widget::InitParams params(
      views::Widget::InitParams::TYPE_WINDOW_FRAMELESS);
  params.opacity = views::Widget::InitParams::TRANSLUCENT_WINDOW;
  params.keep_on_top = true;
  params.ownership = views::Widget::InitParams::WIDGET_OWNS_NATIVE_WIDGET;
  params.name = "PowerOffMenuWindow";
  params.layer_type = ui::LAYER_SOLID_COLOR;
  params.parent = Shell::GetPrimaryRootWindow()->GetChildById(
      kShellWindowId_OverlayContainer);
  fullscreen_widget->Init(params);

  gfx::Rect widget_bounds =
      display::Screen::GetScreen()->GetPrimaryDisplay().bounds();
  fullscreen_widget->SetBounds(widget_bounds);
  fullscreen_widget->Show();
  return fullscreen_widget;
}

void TabletPowerButtonController::SuspendDone(
    const base::TimeDelta& sleep_duration) {
  last_resume_time_ = tick_clock_->NowTicks();
}

}  // namespace ash
