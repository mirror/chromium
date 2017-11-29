// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASH_SYSTEM_POWER_POWER_BUTTON_DISPLAY_CONTROLLER_H_
#define ASH_SYSTEM_POWER_POWER_BUTTON_DISPLAY_CONTROLLER_H_

#include <memory>

#include "ash/ash_export.h"
#include "ash/system/power/display_forced_off_setter.h"
#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "base/scoped_observer.h"
#include "base/time/time.h"
#include "chromeos/dbus/power_manager_client.h"
#include "ui/events/devices/input_device_event_observer.h"
#include "ui/events/event_handler.h"

namespace base {
class TickClock;
}  // namespace base

namespace ash {

class ScopedDisplayForcedOff;

// PowerButtonDisplayController performs display-related tasks (e.g. forcing
// backlights off or disabling the touchscreen) on behalf of
// PowerButtonController and TabletPowerButtonController.
class ASH_EXPORT PowerButtonDisplayController
    : public DisplayForcedOffSetter::Observer,
      public chromeos::PowerManagerClient::Observer,
      public ui::EventHandler,
      public ui::InputDeviceEventObserver {
 public:
  explicit PowerButtonDisplayController(base::TickClock* tick_clock);
  ~PowerButtonDisplayController() override;

  bool IsScreenOn() const;

  base::TimeTicks screen_state_last_changed() const {
    return screen_state_last_changed_;
  }

  // Updates the power manager's backlights-forced-off state and enables or
  // disables the touchscreen. No-op if |backlights_forced_off_| already equals
  // |forced_off|.
  void SetDisplayForcedOff(bool forced_off);

  // Overridden from DisplayForcedOffObserver:
  void OnBacklightsForcedOffChanged() override;

  // Overridden from chromeos::PowerManagerClient::Observer:
  void BrightnessChanged(int level, bool user_initiated) override;
  void SuspendDone(const base::TimeDelta& sleep_duration) override;
  void LidEventReceived(chromeos::PowerManagerClient::LidState state,
                        const base::TimeTicks& timestamp) override;

  // Overridden from ui::EventHandler:
  void OnKeyEvent(ui::KeyEvent* event) override;
  void OnMouseEvent(ui::MouseEvent* event) override;

  // Overridden from ui::InputDeviceObserver:
  void OnStylusStateChanged(ui::StylusState state) override;

 private:
  // Screen state as communicated by D-Bus signals from powerd about backlight
  // brightness changes.
  enum class ScreenState {
    // The screen is on.
    ON,
    // The screen is off.
    OFF,
    // The screen is off, specifically due to an automated change like user
    // inactivity.
    OFF_AUTO,
  };

  // Enables or disables the touchscreen by updating the global touchscreen
  // enabled status. The touchscreen is disabled when backlights are forced off
  // or |screen_state_| is OFF_AUTO.
  void UpdateTouchscreenStatus();

  // Current screen state.
  ScreenState screen_state_ = ScreenState::ON;

  // Saves the most recent timestamp that screen state changed.
  base::TimeTicks screen_state_last_changed_;

  // Time source for performed action times.
  base::TickClock* tick_clock_;  // Not owned.

  // Controls whether the touchscreen is disabled when the screen is turned off
  // due to user inactivity.
  bool disable_touchscreen_while_screen_off_ = true;

  std::unique_ptr<ScopedDisplayForcedOff> display_forced_off_;

  ScopedObserver<DisplayForcedOffSetter, DisplayForcedOffSetter::Observer>
      display_forced_off_observer_;

  base::WeakPtrFactory<PowerButtonDisplayController> weak_ptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(PowerButtonDisplayController);
};

}  // namespace ash

#endif  // ASH_SYSTEM_POWER_POWER_BUTTON_DISPLAY_CONTROLLER_H_
