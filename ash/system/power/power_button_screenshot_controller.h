// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASH_SYSTEM_POWER_POWER_BUTTON_SCREENSHOT_CONTROLLER_H_
#define ASH_SYSTEM_POWER_POWER_BUTTON_SCREENSHOT_CONTROLLER_H_

#include <memory>

#include "ash/ash_export.h"
#include "base/macros.h"
#include "base/time/time.h"
#include "base/timer/timer.h"
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
  // Observes volume key event, implemented and used in tests only.
  class TestObserver {
   public:
    TestObserver() {}
    virtual ~TestObserver() {}

    // Called when volume down key pressed is propagated to other event
    // handlers.
    virtual void OnVolumeDownPressedPropagated() {}

    // Called when volume up key pressed propagated to other event handlers.
    virtual void OnVolumeUpPressedPropagated() {}

   private:
    DISALLOW_COPY_AND_ASSIGN(TestObserver);
  };

  // Time that volume down key and power button must be pressed within this
  // interval of each other to make a screenshot.
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

  // Sets |test_observer_| for testing.
  void SetObserverForTesting(std::unique_ptr<TestObserver> test_observer);

  // If |volume_down_timer_| is running, stops it, runs its task, and returns
  // true. Otherwise returns false.
  bool TriggerVolumeDownTimerForTesting() WARN_UNUSED_RESULT;

  // Returns true if |clamshell_power_button_timer_| is running.
  bool ClamshellPowerButtonTimerIsRunningForTesting() const;

  // If |clamshell_power_button_timer_| is running, stops it, runs its task, and
  // returns true. Otherwise returns false.
  bool TriggerClamshellPowerButtonTimerForTesting() WARN_UNUSED_RESULT;

 private:
  // Helper method used to intercept power button event or volume down key event
  // to check screenshot chord condition.
  void InterceptScreenshotChord();

  // Called by |volume_down_timer_| to perform volume down accelerator.
  void OnVolumeDownTimeout();

  // Called by |clamshell_power_button_timer_| to start clamshell power button
  // behavior.
  void OnClamshellPowerButtonTimeout();

  // True if volume down key is pressed.
  bool volume_down_key_pressed_ = false;

  // True if volume down key is consumed by screenshot accelerator.
  bool volume_down_key_consumed_ = false;

  // True if either volume down or up key is pressed.
  bool volume_key_pressed_ = false;

  // True if power button is pressed.
  bool power_button_pressed_ = false;

  // True if power button is consumed.
  bool power_button_consumed_ = false;

  // Saves the most recent volume down key pressed time.
  base::TimeTicks volume_down_key_pressed_time_;

  // Saves the most recent power button pressed time.
  base::TimeTicks power_button_pressed_time_;

  // Started when volume down key is pressed and power button is not pressed.
  // Stopped when power button is pressed. Runs OnVolumeDownTimeout to perform a
  // volume down accelerator.
  base::OneShotTimer volume_down_timer_;

  // Started when clamshell power button is pressed and volume key is not
  // pressed. Stopped when volume key is pressed or power button is released.
  // Runs OnClamshellTimeout to start clamshell power button behavior.
  base::OneShotTimer clamshell_power_button_timer_;

  TabletPowerButtonController* tablet_controller_;  // Not owned.

  // Time source for performed action times.
  base::TickClock* tick_clock_;  // Not owned.

  // Used for observing volume down key event's stopped propagation. It is
  // nullptr unless set by SetObserverForTesting.
  std::unique_ptr<TestObserver> test_observer_;

  DISALLOW_COPY_AND_ASSIGN(PowerButtonScreenshotController);
};

}  // namespace ash

#endif  // ASH_SYSTEM_POWER_POWER_BUTTON_SCREENSHOT_CONTROLLER_H_
