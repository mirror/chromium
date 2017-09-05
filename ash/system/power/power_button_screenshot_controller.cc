// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/system/power/power_button_screenshot_controller.h"

#include "ash/accelerators/accelerator_controller.h"
#include "ash/shell.h"
#include "ash/system/power/tablet_power_button_controller.h"
#include "base/time/tick_clock.h"
#include "ui/events/event.h"

namespace ash {

namespace {
constexpr int kScreenshotAccelPressedIntervalMs = 150;
}  // namespace

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
  if (down) {
    if (!power_button_triggered_) {
      power_button_triggered_ = true;
      power_button_triggered_time_ = tick_clock_->NowTicks();
      InterceptScreenshotChord();
    }
  } else {
    power_button_triggered_ = false;
  }
  if (power_button_handled_) {
    LOG(ERROR) << "power button handled";
    return true;
  }
  return false;
}

void PowerButtonScreenshotController::OnKeyEvent(ui::KeyEvent* event) {
  LOG(ERROR) << "" ui::KeyboardCode key_code = event->key_code();
  bool down = event->type() == ui::ET_KEY_PRESSED;
  if (key_code == ui::VKEY_VOLUME_DOWN || key_code == ui::VKEY_VOLUME_UP) {
    if (key_code == ui::VKEY_VOLUME_DOWN) {
      if (down) {
        if (!volume_down_key_triggered_) {
          volume_down_key_triggered_ = true;
          volume_down_key_time_ = tick_clock_->NowTicks();
          volume_down_key_consumed_ = false;
          power_button_handled_ = true;
          if (tablet_controller_)
            tablet_controller_->CancelTabletPowerButton();
          InterceptScreenshotChord();
        }
      } else {
        volume_down_key_triggered_ = false;
        power_button_handled_ = false;
      }
    } else {
      // power_button_handled_ = true;
    }
  }

  // if we think ...
  if (volume_down_key_triggered_ && !power_button_triggered_) {
    base::TimeTicks now = tick_clock_->NowTicks();
    base::TimeTicks timeout_time =
        volume_down_key_time_ +
        base::TimeDelta::FromMilliseconds(kScreenshotAccelPressedIntervalMs);
    // if (now <= timeout_time) {
    //// In this case, we think volume down is consumed.
    // event->StopPropagation();
    // return;
    //}
    volume_down_key_consumed_ = (now <= timeout_time);
  }

  if (key_code == ui::VKEY_VOLUME_DOWN && volume_down_key_consumed_) {
    if (!down)
      volume_down_key_consumed_ = false;
    event->StopPropagation();
  }
}

void PowerButtonScreenshotController::InterceptScreenshotChord() {
  if (volume_down_key_triggered_ && power_button_triggered_) {
    base::TimeTicks now = tick_clock_->NowTicks();
    if (now <= volume_down_key_time_ + base::TimeDelta::FromMilliseconds(
                                           kScreenshotAccelPressedIntervalMs) &&
        now <= power_button_triggered_time_ +
                   base::TimeDelta::FromMilliseconds(
                       kScreenshotAccelPressedIntervalMs)) {
      volume_down_key_consumed_ = true;
      Shell::Get()->accelerator_controller()->PerformActionIfEnabled(
          TAKE_SCREENSHOT);
    }
  }
}

}  // namespace ash
