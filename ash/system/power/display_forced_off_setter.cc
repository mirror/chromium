// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/system/power/display_forced_off_setter.h"

#include <vector>

#include "ash/system/power/scoped_display_forced_off.h"
#include "base/bind.h"
#include "base/logging.h"
#include "chromeos/dbus/dbus_thread_manager.h"

namespace ash {

DisplayForcedOffSetter::DisplayForcedOffSetter() : weak_ptr_factory_(this) {
  chromeos::DBusThreadManager::Get()->GetPowerManagerClient()->AddObserver(
      this);

  GetInitialBacklightsForcedOff();
}

DisplayForcedOffSetter::~DisplayForcedOffSetter() {
  chromeos::DBusThreadManager::Get()->GetPowerManagerClient()->RemoveObserver(
      this);

  // Prevent scoped display forced off objects from removing themselves from
  // |active_display_forced_offs_| when they are reset.
  weak_ptr_factory_.InvalidateWeakPtrs();

  for (auto* display_forced_off : active_display_forced_offs_)
    display_forced_off->Reset();
}

void DisplayForcedOffSetter::AddObserver(Observer* observer) {
  observers_.AddObserver(observer);
}

void DisplayForcedOffSetter::RemoveObserver(Observer* observer) {
  observers_.RemoveObserver(observer);
}

std::unique_ptr<ScopedDisplayForcedOff>
DisplayForcedOffSetter::ForceDisplayOff() {
  auto scoped_display_forced_off = std::make_unique<ScopedDisplayForcedOff>(
      base::BindOnce(&DisplayForcedOffSetter::InvalidateScopedDisplayForcedOff,
                     weak_ptr_factory_.GetWeakPtr()));
  active_display_forced_offs_.insert(scoped_display_forced_off.get());
  SetDisplayForcedOff(true);
  return scoped_display_forced_off;
}

void DisplayForcedOffSetter::InvalidateScopedDisplayForcedOff(
    ScopedDisplayForcedOff* scoped_display_forced_off) {
  active_display_forced_offs_.erase(scoped_display_forced_off);
  SetDisplayForcedOff(!active_display_forced_offs_.empty());
}

void DisplayForcedOffSetter::SetDisplayForcedOff(bool forced_off) {
  if (backlights_forced_off_.has_value() &&
      backlights_forced_off_ == forced_off) {
    return;
  }

  backlights_forced_off_ = forced_off;
  // Set the display and keyboard backlights (if present) to |forced_off|.
  chromeos::DBusThreadManager::Get()
      ->GetPowerManagerClient()
      ->SetBacklightsForcedOff(forced_off);

  for (auto& observer : observers_)
    observer.OnBacklightsForcedOffChanged();
}

void DisplayForcedOffSetter::PowerManagerRestarted() {
  if (backlights_forced_off_.has_value()) {
    chromeos::DBusThreadManager::Get()
        ->GetPowerManagerClient()
        ->SetBacklightsForcedOff(backlights_forced_off_.value());
  } else {
    GetInitialBacklightsForcedOff();
  }
}

void DisplayForcedOffSetter::GetInitialBacklightsForcedOff() {
  chromeos::DBusThreadManager::Get()
      ->GetPowerManagerClient()
      ->GetBacklightsForcedOff(base::BindOnce(
          &DisplayForcedOffSetter::OnGotInitialBacklightsForcedOff,
          weak_ptr_factory_.GetWeakPtr()));
}

void DisplayForcedOffSetter::OnGotInitialBacklightsForcedOff(
    base::Optional<bool> is_forced_off) {
  if (backlights_forced_off_.has_value())
    return;
  backlights_forced_off_ = is_forced_off.value_or(false);

  for (auto& observer : observers_)
    observer.OnBacklightsForcedOffChanged();
}

}  // namespace ash
