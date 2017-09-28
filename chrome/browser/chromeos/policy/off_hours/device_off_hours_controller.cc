// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/chromeos/policy/off_hours/device_off_hours_controller.h"

#include <string>
#include <utility>

#include "base/logging.h"
#include "base/optional.h"
#include "base/time/time.h"
#include "chrome/browser/chromeos/policy/off_hours/off_hours_proto_parser.h"
#include "chrome/browser/chromeos/policy/off_hours/utils.h"
#include "chrome/browser/chromeos/settings/cros_settings.h"
#include "chrome/browser/chromeos/settings/device_settings_service.h"
#include "chromeos/dbus/dbus_thread_manager.h"

namespace em = enterprise_management;

namespace policy {

DeviceOffHoursController::DeviceOffHoursController() {
  if (chromeos::DBusThreadManager::IsInitialized()) {
    chromeos::DBusThreadManager::Get()->GetPowerManagerClient()->AddObserver(
        this);
  }
}

DeviceOffHoursController::~DeviceOffHoursController() {
  if (chromeos::DBusThreadManager::IsInitialized()) {
    chromeos::DBusThreadManager::Get()->GetPowerManagerClient()->RemoveObserver(
        this);
  }
}

void DeviceOffHoursController::SuspendDone(
    const base::TimeDelta& sleep_duration) {
  // Triggered when device wakes up. "OffHours" state could be changed during
  // sleep mode and should be updated after that.
  UpdateOffHoursMode();
}

void DeviceOffHoursController::UpdateOffHoursMode() {
  if (off_hours_intervals_.empty()) {
    StopOffHoursTimer();
    SetOffHoursMode(false);
    return;
  }
  off_hours::WeeklyTime current_time =
      off_hours::WeeklyTime::GetCurrentWeeklyTime();
  for (const auto& interval : off_hours_intervals_) {
    if (interval.Contains(current_time)) {
      SetOffHoursMode(true);
      StartOffHoursTimer(current_time.GetDurationTo(interval.end()));
      return;
    }
  }
  StartOffHoursTimer(
      GetDeltaTillNextOffHours(current_time, off_hours_intervals_));
  SetOffHoursMode(false);
}

void DeviceOffHoursController::SetOffHoursMode(bool off_hours_enabled) {
  if (off_hours_mode_ == off_hours_enabled)
    return;
  off_hours_mode_ = off_hours_enabled;
  DVLOG(1) << "OffHours mode: " << off_hours_mode_;
  OffHoursModeIsChanged();
}

void DeviceOffHoursController::StartOffHoursTimer(base::TimeDelta delay) {
  DCHECK_GT(delay, base::TimeDelta());
  DVLOG(1) << "OffHours mode timer starts for " << delay;
  timer_.Start(FROM_HERE, delay,
               base::Bind(&DeviceOffHoursController::UpdateOffHoursMode,
                          base::Unretained(this)));
}

void DeviceOffHoursController::StopOffHoursTimer() {
  timer_.Stop();
}

void DeviceOffHoursController::OffHoursModeIsChanged() {
  DVLOG(1) << "OffHours mode is changed.";
  // TODO(yakovleva): Get discussion about what is better to user Load() or
  // LoadImmediately().
  chromeos::DeviceSettingsService::Get()->Load();
}

bool DeviceOffHoursController::IsOffHoursMode() {
  return off_hours_mode_;
}

void DeviceOffHoursController::UpdateOffHoursPolicy(
    const em::ChromeDeviceSettingsProto& device_settings_proto) {
  std::vector<off_hours::OffHoursInterval> off_hours_intervals;
  if (device_settings_proto.has_device_off_hours()) {
    const em::DeviceOffHoursProto& container(
        device_settings_proto.device_off_hours());
    base::Optional<std::string> timezone = off_hours::GetTimezone(container);
    if (timezone) {
      off_hours_intervals = off_hours::ConvertIntervalsToGmt(
          off_hours::GetIntervals(container), *timezone);
    }
  }
  off_hours_intervals_.swap(off_hours_intervals);
  UpdateOffHoursMode();
}

}  // namespace policy
