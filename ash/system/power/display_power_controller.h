// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASH_SYSTEM_POWER_DISPLAY_POWER_CONTROLLER_H_
#define ASH_SYSTEM_POWER_DISPLAY_POWER_CONTROLLER_H_

#include <memory>
#include <set>

#include "ash/ash_export.h"
#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "base/observer_list.h"
#include "base/optional.h"
#include "base/time/time.h"
#include "chromeos/dbus/power_manager_client.h"
#include "ui/events/devices/input_device_event_observer.h"
#include "ui/events/event_handler.h"

namespace ash {

class DisplayPowerControllerObserver;
class ScopedDisplayForcedOff;

// DisplayPowerController performs display-related tasks (e.g. forcing
// backlights off or disabling the touchscreen) on behalf of
// PowerButtonController and TabletPowerButtonController.
class ASH_EXPORT DisplayPowerController
    : public chromeos::PowerManagerClient::Observer,
      public ui::EventHandler,
      public ui::InputDeviceEventObserver {
 public:
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

  DisplayPowerController();
  ~DisplayPowerController() override;

  void AddObserver(DisplayPowerControllerObserver* observer);
  void RemoveObserver(DisplayPowerControllerObserver* observer);

  ScreenState screen_state() const { return screen_state_; }

  bool backlights_forced_off() const { return backlights_forced_off_; }

  // Updates the power manager's backlights-forced-off state and enables or
  // disables the touchscreen. No-op if |backlights_forced_off_| already equals
  // |forced_off|.
  std::unique_ptr<ScopedDisplayForcedOff> ForceDisplayOff();

  // Overridden from chromeos::PowerManagerClient::Observer:
  void PowerManagerRestarted() override;
  void BrightnessChanged(int level, bool user_initiated) override;
  void SuspendDone(const base::TimeDelta& sleep_duration) override;
  void LidEventReceived(chromeos::PowerManagerClient::LidState state,
                        const base::TimeTicks& timestamp) override;

  // Overridden from ui::EventHandler:
  void OnKeyEvent(ui::KeyEvent* event) override;
  void OnMouseEvent(ui::MouseEvent* event) override;

  // Overridden from ui::InputDeviceObserver:
  void OnStylusStateChanged(ui::StylusState state) override;

  // Called from PowerButtonController when the power button event is detected.
  void OnPowerButtonEvent(bool down, const base::TimeTicks& time_stamp);

 private:
  friend class TabletPowerButtonControllerTestApi;

  // Sends a request to powerd to get the backlights forced off state so that
  // |backlights_forced_off_| can be initialized.
  void GetInitialBacklightsForcedOff();

  // Initializes |backlights_forced_off_|.
  void OnGotInitialBacklightsForcedOff(base::Optional<bool> is_forced_off);

  // Enables or disables the touchscreen by updating the global touchscreen
  // enabled status. The touchscreen is disabled when backlights are forced off
  // or |screen_state_| is OFF_AUTO.
  void UpdateTouchscreenStatus();

  void UnregisterDisplayForcedOff(
      ScopedDisplayForcedOff* scoped_display_forced_off);
  void CancelDisplayForcedOffForUserAction();
  void SetDisplayForcedOff(bool forced_off);

  // Current screen state.
  ScreenState screen_state_ = ScreenState::ON;

  // Current forced-off state of backlights.
  bool backlights_forced_off_ = false;

  // Controls whether the touchscreen is disabled when the screen is turned off
  // due to user inactivity.
  bool disable_touchscreen_while_screen_off_ = true;

  std::set<ScopedDisplayForcedOff*> active_display_forced_offs_;

  base::ObserverList<DisplayPowerControllerObserver> observers_;

  base::WeakPtrFactory<DisplayPowerController> weak_ptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(DisplayPowerController);
};

}  // namespace ash

#endif  // ASH_SYSTEM_POWER_DISPLAY_POWER_CONTROLLER_H_
