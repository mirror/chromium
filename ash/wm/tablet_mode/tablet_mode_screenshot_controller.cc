// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/wm/tablet_mode/tablet_mode_screenshot_controller.h"

#include "ash/accelerators/accelerator_controller.h"
#include "ash/shell.h"
#include "ash/wm/tablet_mode/tablet_mode_controller.h"
#include "base/time/default_tick_clock.h"
#include "chromeos/dbus/dbus_thread_manager.h"
#include "ui/events/event.h"

namespace ash {

namespace {
constexpr int kScreenshotAccelPressedIntervalMs = 150;
}  // namespace

TabletModeScreenshotController::TabletModeScreenshotController()
    : tick_clock_(new base::DefaultTickClock) {
  chromeos::DBusThreadManager::Get()->GetPowerManagerClient()->AddObserver(
      this);
  Shell::Get()->PrependPreTargetHandler(this);
}

TabletModeScreenshotController::~TabletModeScreenshotController() {
  Shell::Get()->RemovePreTargetHandler(this);
  chromeos::DBusThreadManager::Get()->GetPowerManagerClient()->RemoveObserver(
      this);
}

bool TabletModeScreenshotController::ScreenshotHandled() {
  if (!Shell::Get()
           ->tablet_mode_controller()
           ->IsTabletModeWindowManagerEnabled()) {
    return false;
  }

  base::TimeDelta delta;
  if (last_power_button_down_time_ > last_volume_down_time_)
    delta = last_power_button_down_time_ - last_volume_down_time_;
  else
    delta = last_volume_down_time_ - last_power_button_down_time_;

  if (delta >
      base::TimeDelta::FromMilliseconds(kScreenshotAccelPressedIntervalMs))
    return false;

  Shell::Get()->accelerator_controller()->PerformActionIfEnabled(
      TAKE_SCREENSHOT);
  return true;
}

void TabletModeScreenshotController::PowerButtonEventReceived(
    bool down,
    const base::TimeTicks& timestamp) {
  if (down)
    last_power_button_down_time_ = timestamp;
}

void TabletModeScreenshotController::OnKeyEvent(ui::KeyEvent* event) {
  if (event->key_code() == ui::VKEY_VOLUME_DOWN)
    last_volume_down_time_ = tick_clock_->NowTicks();
}

}  // namespace ash
