// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASH_SYSTEM_POWER_BACKLIGHTS_FORCED_OFF_SETTER_H_
#define ASH_SYSTEM_POWER_BACKLIGHTS_FORCED_OFF_SETTER_H_

#include <memory>
#include <set>

#include "ash/ash_export.h"
#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "base/observer_list.h"
#include "base/optional.h"
#include "base/scoped_observer.h"
#include "chromeos/dbus/power_manager_client.h"

namespace ash {

class ScopedBacklightsForcedOff;

// BacklightsForcedOffSetter manages multiple requests to force the backlights
// off and coalesces them into SetBacklightsForcedOff D-Bus calls to powerd.
class ASH_EXPORT BacklightsForcedOffSetter
    : public chromeos::PowerManagerClient::Observer {
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

  class Observer {
   public:
    virtual ~Observer() = default;

    // Called when the observed BacklightsForcedOffSetter instance stops or
    // starts forcing backlights off.
    virtual void OnBacklightsForcedOffChanged(bool backlights_forced_off) {}

    // Called when the screen state change is detected.
    virtual void OnScreenStateChanged() {}
  };

  BacklightsForcedOffSetter();
  ~BacklightsForcedOffSetter() override;

  void AddObserver(Observer* observer);
  void RemoveObserver(Observer* observer);

  bool backlights_forced_off() const {
    return backlights_forced_off_.value_or(false);
  }

  ScreenState screen_state() const { return screen_state_; }

  // Forces the backlights off. The backlights will be kept in the forced-off
  // state until the all requests have been destroyed. Note: the
  // BacklightsForcedOffSetter keeps track of number of active
  // |ScopedBacklightsForcedOff| objects returned by this method - it passes
  // |InvalidateScopedBacklightsForcesOff| to the the returned object as a
  // callback invoked when the object is reset. Backlights will remain in
  // forced-off state as long as at least one active |ScopedBacklightsForcedOff|
  // object exists.
  std::unique_ptr<ScopedBacklightsForcedOff> ForceBacklightsOff();

  // Overridden from chromeos::PowerManagerClient::Observer:
  void BrightnessChanged(int level, bool user_initiated) override;
  void PowerManagerRestarted() override;

 private:
  // For access to |ResetForTest()|.
  friend class ShellTestApi;

  // Resets internal state for tests.
  // Note: This will silently cancel all active backlights forced off requests.
  void ResetForTest();

  // Sets |disable_touchscreen_while_screen_off_| depending on the state of the
  // current command line,
  void InitDisableTouchscreenWhileScreenOff();

  // Sends a request to powerd to get the backlights forced off state so that
  // |backlights_forced_off_| can be initialized.
  void GetInitialBacklightsForcedOff();

  // Initializes |backlights_forced_off_|.
  void OnGotInitialBacklightsForcedOff(base::Optional<bool> is_forced_off);

  // Removes a force backlights off request from the list of active ones, which
  // effectively cancels the request. This is passed to every
  // ScopedBacklightsForcedOff created by |ForceBacklightsOff| as its
  // cancellation callback.
  void InvalidateScopedBacklightsForcedOff();

  // Updates the power manager's backlights-forced-off state.
  void SetBacklightsForcedOff(bool forced_off);

  // Enables or disables the touchscreen by updating the global touchscreen
  // enabled status. The touchscreen is disabled when backlights are forced off
  // or |screen_state_| is OFF_AUTO.
  void UpdateTouchscreenStatus();

  // Controls whether the touchscreen is disabled when the screen is turned off
  // due to user inactivity.
  bool disable_touchscreen_while_screen_off_ = true;

  // Current forced-off state of backlights.
  base::Optional<bool> backlights_forced_off_;

  // Current screen state.
  ScreenState screen_state_ = ScreenState::ON;

  // Number of active backlights forced off requests.
  int active_backlights_forced_off_count_ = 0;

  base::ObserverList<Observer> observers_;

  ScopedObserver<chromeos::PowerManagerClient,
                 chromeos::PowerManagerClient::Observer>
      power_manager_observer_;

  base::WeakPtrFactory<BacklightsForcedOffSetter> weak_ptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(BacklightsForcedOffSetter);
};

}  // namespace ash

#endif  // ASH_SYSTEM_POWER_BACKLIGHTS_FORCED_OFF_SETTER_H_
