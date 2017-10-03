// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/system/power/power_button_screenshot_controller.h"

#include "ash/accelerators/accelerator_controller.h"
#include "ash/shell.h"
#include "ash/system/power/tablet_power_button_controller.h"
#include "ash/wm/tablet_mode/tablet_mode_controller.h"
#include "base/time/tick_clock.h"
#include "ui/events/event.h"

namespace ash {

namespace {

bool IsTabletMode() {
  return Shell::Get()
      ->tablet_mode_controller()
      ->IsTabletModeWindowManagerEnabled();
}

}  // namespace

constexpr base::TimeDelta
    PowerButtonScreenshotController::kScreenshotChordDelay;

PowerButtonScreenshotController::PowerButtonScreenshotController(
    TabletPowerButtonController* tablet_controller,
    base::TickClock* tick_clock)
    : tablet_controller_(tablet_controller), tick_clock_(tick_clock) {
  Shell::Get()->PrependPreTargetHandler(this);
}

PowerButtonScreenshotController::~PowerButtonScreenshotController() {
  Shell::Get()->RemovePreTargetHandler(this);
}

bool PowerButtonScreenshotController::OnPowerButtonEvent(
    bool down,
    const base::TimeTicks& timestamp) {
  DCHECK(IsTabletMode());
  power_button_consumed_ = false;
  power_button_pressed_ = down;
  if (power_button_pressed_) {
    volume_down_timer_.Stop();

    power_button_pressed_time_ = tick_clock_->NowTicks();
    InterceptScreenshotChord();
  }

  // If volume key is pressed, set power button as consumed. This invalidates
  // other power button's behavior when user tries to operate screenshot.
  if (volume_key_pressed_)
    power_button_consumed_ = true;

  return power_button_consumed_;
}

void PowerButtonScreenshotController::OnKeyEvent(ui::KeyEvent* event) {
  if (!IsTabletMode())
    return;

  ui::KeyboardCode key_code = event->key_code();
  if (key_code != ui::VKEY_VOLUME_DOWN && key_code != ui::VKEY_VOLUME_UP)
    return;

  bool down = event->type() == ui::ET_KEY_PRESSED;
  volume_key_pressed_ = down;
  // When volume key is pressed, cancel the ongoing tablet power button
  // behavior.
  if (volume_key_pressed_ && tablet_controller_)
    tablet_controller_->CancelTabletPowerButton();

  if (key_code == ui::VKEY_VOLUME_DOWN) {
    volume_down_key_pressed_ = down;
    if (volume_down_key_pressed_ && !event->is_repeat()) {
      volume_down_key_pressed_time_ = tick_clock_->NowTicks();
      volume_down_key_consumed_ = false;
      InterceptScreenshotChord();
    }

    // Do not propagate volume down key event if it is consumed by screenshot.
    if (volume_down_key_consumed_) {
      event->StopPropagation();
      return;
    }
  }

  // On volume down key pressed while power button not pressed yet state, do not
  // propagate volume down key event for chord delay time. Start the timer to
  // wait power button pressed for screenshot operation, and on timeout perform
  // the delayed volume down operation.
  if (volume_down_key_pressed_ && !power_button_pressed_) {
    base::TimeTicks now = tick_clock_->NowTicks();
    if (now <= volume_down_key_pressed_time_ + kScreenshotChordDelay) {
      event->StopPropagation();

      if (!volume_down_timer_.IsRunning()) {
        volume_down_timer_.Start(
            FROM_HERE, kScreenshotChordDelay, this,
            &PowerButtonScreenshotController::OnVolumeDownTimeout);
      }
    }
  }
}

void PowerButtonScreenshotController::InterceptScreenshotChord() {
  if (volume_down_key_pressed_ && power_button_pressed_) {
    base::TimeTicks now = tick_clock_->NowTicks();
    if (now <= volume_down_key_pressed_time_ + kScreenshotChordDelay &&
        now <= power_button_pressed_time_ + kScreenshotChordDelay) {
      volume_down_key_consumed_ = true;
      power_button_consumed_ = true;
      Shell::Get()->accelerator_controller()->PerformActionIfEnabled(
          TAKE_SCREENSHOT);
    }
  }
}

void PowerButtonScreenshotController::OnVolumeDownTimeout() {
  Shell::Get()->accelerator_controller()->PerformActionIfEnabled(VOLUME_DOWN);
}

}  // namespace ash
