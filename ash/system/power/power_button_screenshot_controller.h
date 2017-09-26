// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASH_SYSTEM_POWER_POWER_BUTTON_SCREENSHOT_CONTROLLER_H_
#define ASH_SYSTEM_POWER_POWER_BUTTON_SCREENSHOT_CONTROLLER_H_

#include <memory>

#include "ash/ash_export.h"
#include "base/macros.h"
#include "base/time/time.h"
#include "ui/events/event_handler.h"

namespace base {
class TickClock;
}  // namespace base

namespace ash {

class TabletPowerButtonController;

// Handles power button screenshot accelerator. This class is instantiated and
// used in PowerButtonController.
class ASH_EXPORT PowerButtonScreenshotController : public ui::EventHandler {
 public:
  // Time that volume down and power button must be pressed within this interval
  // of each other to make a screenshot.
  static constexpr base::TimeDelta kScreenshotChordDelay =
      base::TimeDelta::FromMilliseconds(150);

  PowerButtonScreenshotController(
      TabletPowerButtonController* tablet_controller,
      base::TickClock* tick_clock);
  ~PowerButtonScreenshotController() override;

  // Returns true if power button event is consumed by |this|, otherwise false.
  bool OnPowerButtonEvent(bool down, const base::TimeTicks& timestamp);

  // Overridden from ui::EventHandler:
  void OnKeyEvent(ui::KeyEvent* event) override;

  bool volume_key_pressed() const { return volume_key_pressed_; }

 private:
  // Helper method used to intercept power button event or volume down key event
  // to check screenshot chord condition.
  void InterceptScreenshotChord();

  // Returns true if volume down key is pressed.
  bool volume_down_key_pressed_ = false;

  // Returns true if the volume down key is consumed by screenshot accelerator.
  bool volume_down_key_consumed_ = false;

  // Returns true if either volume down or up key is pressed.
  bool volume_key_pressed_ = false;

  // Returns true if power button is pressed.
  bool power_button_pressed_ = false;

  // Returns true if power button is consumed.
  bool power_button_consumed_ = false;

  // Saves the most recent volume down key pressed time.
  base::TimeTicks volume_down_key_pressed_time_;

  // Saves the most recent power button pressed time.
  base::TimeTicks power_button_pressed_time_;

  TabletPowerButtonController* tablet_controller_;  // Not owned.

  // Time source for performed action times.
  base::TickClock* tick_clock_;  // Not owned.

  DISALLOW_COPY_AND_ASSIGN(PowerButtonScreenshotController);
};

}  // namespace ash

#endif  // ASH_SYSTEM_POWER_POWER_BUTTON_SCREENSHOT_CONTROLLER_H_
