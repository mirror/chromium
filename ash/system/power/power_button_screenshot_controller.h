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

class ASH_EXPORT PowerButtonScreenshotController : public ui::EventHandler {
 public:
  PowerButtonScreenshotController(
      TabletPowerButtonController* tablet_controller,
      base::TickClock* tick_clock);
  ~PowerButtonScreenshotController() override;

  bool OnPowerButtonEvent(bool down, const base::TimeTicks& timestamp);

  // Overridden from ui::EventHandler:
  void OnKeyEvent(ui::KeyEvent* event) override;

 private:
  void InterceptScreenshotChord();

  bool volume_down_key_triggered_ = false;

  bool volume_down_key_consumed_ = false;

  bool power_button_triggered_ = false;

  bool power_button_handled_ = false;

  base::TimeTicks volume_down_key_time_;
  base::TimeTicks power_button_triggered_time_;

  TabletPowerButtonController* tablet_controller_;  // Not owned.

  base::TickClock* tick_clock_;  // Not owned.

  DISALLOW_COPY_AND_ASSIGN(PowerButtonScreenshotController);
};

}  // namespace ash

#endif  // ASH_SYSTEM_POWER_POWER_BUTTON_SCREENSHOT_CONTROLLER_H_
