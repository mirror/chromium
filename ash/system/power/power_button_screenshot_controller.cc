// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/system/power/power_button_screenshot_controller.h"

#include "ash/accelerators/accelerator_controller.h"
#include "ash/public/cpp/ash_switches.h"
#include "ash/shell.h"
#include "ash/system/power/tablet_power_button_controller.h"
#include "ash/wm/power_button_controller.h"
#include "ash/wm/tablet_mode/tablet_mode_controller.h"
#include "base/command_line.h"
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
  if (!IsTabletMode())
    return false;

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

  // If forced-clamshell power button releases when
  // |clamshell_power_button_timer_| is still running, stop the timer to
  // invalidate this delayed power button behavior.
  if (!down) {
    clamshell_power_button_timer_.Stop();
    return false;
  }

  const bool forced_clamshell =
      base::CommandLine::ForCurrentProcess()->HasSwitch(
          switches::kForceClamshellPowerButton);
  // If |forced_clamshell|, start a timer to wait volume down key pressed. If
  // timer timeouts, perform this delayed clamshell power button behavior.
  if (forced_clamshell && !volume_down_key_pressed_ && down) {
    clamshell_power_button_timer_.Start(
        FROM_HERE, kScreenshotChordDelay, this,
        &PowerButtonScreenshotController::OnClamshellPowerButtonTimeout);
    return true;
  }

  return power_button_consumed_;
}

void PowerButtonScreenshotController::OnKeyEvent(ui::KeyEvent* event) {
  if (!IsTabletMode())
    return;

  ui::KeyboardCode key_code = event->key_code();
  if (key_code != ui::VKEY_VOLUME_DOWN && key_code != ui::VKEY_VOLUME_UP)
    return;
  clamshell_power_button_timer_.Stop();

  bool down = event->type() == ui::ET_KEY_PRESSED;
  volume_key_pressed_ = down;
  // When volume key is pressed, cancel the ongoing tablet power button
  // behavior.
  if (volume_key_pressed_ && tablet_controller_)
    tablet_controller_->CancelTabletPowerButton();

  bool volume_down_stopped_propagation = false;
  if (key_code == ui::VKEY_VOLUME_DOWN) {
    if (down) {
      if (!volume_down_key_pressed_) {
        volume_down_key_pressed_ = true;
        volume_down_key_pressed_time_ = tick_clock_->NowTicks();
        volume_down_key_consumed_ = false;
        InterceptScreenshotChord();
      }
    } else {
      volume_down_key_pressed_ = false;
    }

    // Do not propagate volume down key event if it is consumed by screenshot.
    if (volume_down_key_pressed_ && volume_down_key_consumed_) {
      event->StopPropagation();
      volume_down_stopped_propagation = true;
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
      volume_down_stopped_propagation = true;

      if (!volume_down_timer_.IsRunning()) {
        volume_down_timer_.Start(
            FROM_HERE, kScreenshotChordDelay, this,
            &PowerButtonScreenshotController::OnVolumeDownTimeout);
      }
    }
  }

  if (test_observer_ && down) {
    if (key_code == ui::VKEY_VOLUME_DOWN && !volume_down_stopped_propagation)
      test_observer_->OnVolumeDownPressedPropagated();
    else if (key_code == ui::VKEY_VOLUME_UP)
      test_observer_->OnVolumeUpPressedPropagated();
  }
}

void PowerButtonScreenshotController::SetObserverForTesting(
    std::unique_ptr<TestObserver> test_observer) {
  test_observer_ = std::move(test_observer);
}

bool PowerButtonScreenshotController::TriggerVolumeDownTimerForTesting() {
  if (!volume_down_timer_.IsRunning())
    return false;

  base::Closure task = volume_down_timer_.user_task();
  volume_down_timer_.Stop();
  task.Run();
  return true;
}

bool PowerButtonScreenshotController::
    ClamshellPowerButtonTimerIsRunningForTesting() const {
  return clamshell_power_button_timer_.IsRunning();
}

bool PowerButtonScreenshotController::
    TriggerClamshellPowerButtonTimerForTesting() {
  if (!clamshell_power_button_timer_.IsRunning())
    return false;

  base::Closure task = clamshell_power_button_timer_.user_task();
  clamshell_power_button_timer_.Stop();
  task.Run();
  return true;
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
  if (test_observer_)
    test_observer_->OnVolumeDownPressedPropagated();
  Shell::Get()->accelerator_controller()->PerformActionIfEnabled(VOLUME_DOWN);
}

void PowerButtonScreenshotController::OnClamshellPowerButtonTimeout() {
  Shell::Get()->power_button_controller()->OnPowerButtonEvent(
      true, tick_clock_->NowTicks());
}

}  // namespace ash
