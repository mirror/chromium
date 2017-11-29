// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASH_SYSTEM_POWER_DISPLAY_FORCED_OFF_SETTER_H_
#define ASH_SYSTEM_POWER_DISPLAY_FORCED_OFF_SETTER_H_

#include <memory>
#include <set>

#include "ash/ash_export.h"
#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "base/observer_list.h"
#include "base/optional.h"
#include "chromeos/dbus/power_manager_client.h"

namespace ash {

class ScopedDisplayForcedOff;

// DisplayPowerController performs display-related tasks (e.g. forcing
// backlights off or disabling the touchscreen) on behalf of
// PowerButtonController and TabletPowerButtonController.
class ASH_EXPORT DisplayForcedOffSetter
    : public chromeos::PowerManagerClient::Observer {
 public:
  class Observer {
   public:
    virtual ~Observer() = default;

    // Called when DisplayPowerController starts or stops forcing backlights
    // off.
    virtual void OnBacklightsForcedOffChanged() {}
  };

  DisplayForcedOffSetter();
  ~DisplayForcedOffSetter() override;

  void AddObserver(Observer* observer);
  void RemoveObserver(Observer* observer);

  bool backlights_forced_off_set() const {
    return backlights_forced_off_.has_value();
  }
  bool backlights_forced_off() const {
    return backlights_forced_off_.value_or(false);
  }

  // Forces the display off. The display will be kept in the forced-off state
  // until the returned object is reset.
  // Note: the DisplayForcedOffSetter keeps track of active
  // |ScopedDisplayForcedOff| objects returned by this method - it passes
  // |InvalidateScopedDisplayForcesOff| to the the returned object as a callback
  // invoked when the object is reset. Display will remain in forced-off state
  // as long as at least one active |ScopedDisplayForcedOff| object exists.
  std::unique_ptr<ScopedDisplayForcedOff> ForceDisplayOff();

  // Overridden from chromeos::PowerManagerClient::Observer:
  void PowerManagerRestarted() override;

 private:
  // Sends a request to powerd to get the backlights forced off state so that
  // |backlights_forced_off_| can be initialized.
  void GetInitialBacklightsForcedOff();

  // Initializes |backlights_forced_off_|.
  void OnGotInitialBacklightsForcedOff(base::Optional<bool> is_forced_off);

  // Removes a force display off request from the list of active ones, which
  // effectively cancels the request. This is passed to every
  // ScopedDisplayForcedOff created by |ForceDisplayOff| as its cancellation
  // callback.
  // |scoped_display_forced_off| to invalidate.
  void InvalidateScopedDisplayForcedOff(
      ScopedDisplayForcedOff* scoped_display_forced_off);

  // Updates the power manager's backlights-forced-off state.
  void SetDisplayForcedOff(bool forced_off);

  // Current forced-off state of backlights.
  base::Optional<bool> backlights_forced_off_;

  std::set<ScopedDisplayForcedOff*> active_display_forced_offs_;

  base::ObserverList<Observer> observers_;

  base::WeakPtrFactory<DisplayForcedOffSetter> weak_ptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(DisplayForcedOffSetter);
};

}  // namespace ash

#endif  // ASH_SYSTEM_POWER_DISPLAY_FORCED_OFF_SETTER_H_
