// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/chromeos/policy/device_off_hours_controller.h"

#include <string>
#include <tuple>
#include <utility>
#include <vector>

#include "base/memory/ptr_util.h"
#include "base/strings/utf_string_conversions.h"
#include "base/time/default_clock.h"
#include "base/time/time.h"
#include "chrome/browser/chromeos/policy/device_policy_remover.h"
#include "chrome/browser/chromeos/policy/off_hours/off_hours_interval.h"
#include "chrome/browser/chromeos/settings/cros_settings.h"
#include "chrome/browser/chromeos/settings/device_settings_service.h"
#include "third_party/icu/source/i18n/unicode/gregocal.h"

namespace em = enterprise_management;

namespace policy {

using namespace weekly_time;

namespace {

constexpr base::TimeDelta kMinute = base::TimeDelta::FromMinutes(1);
constexpr base::TimeDelta kHour = base::TimeDelta::FromHours(1);
constexpr base::TimeDelta kDay = base::TimeDelta::FromDays(1);
constexpr base::TimeDelta kWeek = base::TimeDelta::FromDays(7);

// Put time in milliseconds which is added to GMT to get local time to |offset|
// considering current daylight time. Return true if there was no error.
bool GetTimezoneOffset(const std::string& timezone,
                       base::Clock* clock,
                       int* offset) {
  auto zone = base::WrapUnique(
      icu::TimeZone::createTimeZone(icu::UnicodeString::fromUTF8(timezone)));
  if (*zone == icu::TimeZone::getUnknown()) {
    LOG(ERROR) << "Timezone error";
    return false;
  }
  // Time in milliseconds which is added to GMT to get local time.
  int gmt_offset = zone->getRawOffset();
  // Time in milliseconds which is added to local standard time to get local
  // wall clock time.
  int dst_offset = zone->getDSTSavings();
  UErrorCode status = U_ZERO_ERROR;
  std::unique_ptr<icu::GregorianCalendar> gregorian_calendar =
      base::MakeUnique<icu::GregorianCalendar>(*zone, status);
  if (U_FAILURE(status)) {
    LOG(ERROR) << "Gregorian calendar error = " << u_errorName(status);
    return false;
  }
  UDate cur_date = static_cast<UDate>(clock->Now().ToDoubleT() * 1000);
  status = U_ZERO_ERROR;
  gregorian_calendar->setTime(cur_date, status);
  if (U_FAILURE(status)) {
    LOG(ERROR) << "Gregorian calendar set time error = " << u_errorName(status);
    return false;
  }
  status = U_ZERO_ERROR;
  UBool day_light = gregorian_calendar->inDaylightTime(status);
  if (U_FAILURE(status)) {
    LOG(ERROR) << "Daylight time error = " << u_errorName(status);
    return false;
  }
  if (day_light)
    gmt_offset += dst_offset;
  *offset = gmt_offset;
  return true;
}

// Get and return WeeklyTime structure from WeeklyTimeProto. Return nullptr if
// WeeklyTime structure isn't correct.
std::unique_ptr<WeeklyTime> GetWeeklyTime(const em::WeeklyTimeProto& container,
                                          int gmt_offset) {
  if (!container.has_day_of_week() ||
      container.day_of_week() == em::WeeklyTimeProto::DAY_OF_WEEK_UNSPECIFIED) {
    LOG(ERROR) << "Day of week in interval is absent or unspecified.";
    return nullptr;
  }
  if (!container.has_time()) {
    LOG(ERROR) << "Time in interval is absent.";
    return nullptr;
  }
  int time_of_day = container.time();
  if (!(time_of_day >= 0 && time_of_day < kDay.InMilliseconds())) {
    LOG(ERROR) << "Invalid time value: " << time_of_day
               << ", the value should be in [0; " << kDay.InMilliseconds()
               << ").";
    return nullptr;
  }
  WeeklyTime weekly_time =
      ConvertToUTCWeeklyTime(container.day_of_week(), time_of_day, gmt_offset);
  return base::MakeUnique<WeeklyTime>(weekly_time.getDayOfWeek(),
                                      weekly_time.getMilliseconds());
}

// Get and return list of time intervals from DeviceOffHoursProto structure.
// If |convert_gmt_offset| > 0 then intervals convert to UTC timezone.
std::vector<Interval> GetIntervals(const em::DeviceOffHoursProto& container,
                                   int convert_gmt_offset) {
  std::vector<Interval> intervals;
  for (const auto& entry : container.intervals()) {
    if (!entry.has_start() || !entry.has_end()) {
      LOG(WARNING) << "Skipping interval without start or/and end.";
      continue;
    }
    auto start = GetWeeklyTime(entry.start(), convert_gmt_offset);
    auto end = GetWeeklyTime(entry.end(), convert_gmt_offset);
    if (start && end)
      intervals.push_back(Interval(*start, *end));
  }
  return intervals;
}

// Get and return list of ignored policies from DeviceOffHoursProto structure.
std::vector<std::string> GetIgnoredPolicies(
    const em::DeviceOffHoursProto& container) {
  return std::vector<std::string>(container.ignored_policies().begin(),
                                  container.ignored_policies().end());
}

// If |input_policies| contains DeviceOffHoursProto
// then convert intervals to UTC timezone and return list of "OffHours"
// intervals else return empty list.
std::vector<Interval> GetIntervalsIfExists(
    const em::ChromeDeviceSettingsProto& input_policies,
    base::Clock* clock) {
  if (!input_policies.has_device_off_hours()) {
    return {};
  }
  const em::DeviceOffHoursProto& container(input_policies.device_off_hours());
  if (!container.has_timezone()) {
    return {};
  }
  int gmt_offset;
  bool no_offset_error =
      GetTimezoneOffset(container.timezone(), clock, &gmt_offset);
  if (!no_offset_error)
    return {};
  return GetIntervals(container, gmt_offset);
}

}  // namespace

std::unique_ptr<base::DictionaryValue> ConvertOffHoursProtoToValue(
    const em::DeviceOffHoursProto& container) {
  if (!container.has_timezone())
    return nullptr;
  auto off_hours = base::MakeUnique<base::DictionaryValue>();
  off_hours->SetString("timezone", container.timezone());
  std::vector<Interval> intervals = GetIntervals(container, 0);
  auto intervals_value = base::MakeUnique<base::ListValue>();
  for (const auto& interval : intervals)
    intervals_value->Append(interval.ToValue());
  off_hours->SetList("intervals", std::move(intervals_value));
  std::vector<std::string> ignored_policies = GetIgnoredPolicies(container);
  auto ignored_policies_value = base::MakeUnique<base::ListValue>();
  for (const auto& policy : ignored_policies)
    ignored_policies_value->AppendString(policy);
  off_hours->SetList("ignored_policies", std::move(ignored_policies_value));
  return off_hours;
}

std::unique_ptr<em::ChromeDeviceSettingsProto> ApplyOffHoursPolicyToProto(
    const em::ChromeDeviceSettingsProto& input_policies) {
  const em::DeviceOffHoursProto& container(input_policies.device_off_hours());
  std::vector<std::string> ignored_policies = GetIgnoredPolicies(container);
  std::unique_ptr<em::ChromeDeviceSettingsProto> policies =
      base::MakeUnique<em::ChromeDeviceSettingsProto>(input_policies);
  RemovePolicies(policies.get(), ignored_policies);
  return policies;
}

bool IsCurrentUserAllowedBeyondOffHours() {
  // TODO(yakovleva): Return false only if user isn't allowed after "OffHours"
  // mode.
  return false;
}

// Observer
DeviceOffHoursController::Observer::~Observer() {}

void DeviceOffHoursController::Observer::OnOffHoursModeNeedsUpdate() {}

void DeviceOffHoursController::AddObserver(Observer* observer) {
  observers_.AddObserver(observer);
}

void DeviceOffHoursController::RemoveObserver(Observer* observer) {
  observers_.RemoveObserver(observer);
}

DeviceOffHoursController::DeviceOffHoursController() {
  clock_.reset(new base::DefaultClock());
  timer_.reset(new base::OneShotTimer());
  off_hours_mode_ = false;
  net::NetworkChangeNotifier::AddNetworkChangeObserver(this);
}

DeviceOffHoursController::DeviceOffHoursController(
    base::Clock* clock,
    base::TickClock* timer_clock) {
  clock_.reset(clock);
  timer_.reset(new base::OneShotTimer(timer_clock));
  off_hours_mode_ = false;
  // TODO(yakovleva): Find observer which notifies that device wakes up
  // including time when device is offline.
  net::NetworkChangeNotifier::AddNetworkChangeObserver(this);
}

DeviceOffHoursController::~DeviceOffHoursController() {
  net::NetworkChangeNotifier::RemoveNetworkChangeObserver(this);
  StopOffHoursTimer();
}

bool DeviceOffHoursController::IsOffHoursMode() {
  return off_hours_mode_;
}

void DeviceOffHoursController::SetOffHoursMode(bool off_hours_enabled) {
  if (off_hours_mode_ == off_hours_enabled)
    return;
  off_hours_mode_ = off_hours_enabled;
  if (off_hours_mode_)
    VLOG(1) << "OffHours mode is on.";
  else
    VLOG(1) << "OffHours mode is off.";
}

void DeviceOffHoursController::StartOffHoursTimer(base::TimeDelta delay) {
  DCHECK_GT(delay.InMilliseconds(), 0);
  VLOG(1) << "OffHours mode timer starts for "
          << delay.InMilliseconds() / kMinute.InMilliseconds() << " minutes.";
  timer_->Start(
      FROM_HERE, delay,
      base::Bind(&DeviceOffHoursController::NotifyOffHoursModeNeedsUpdate,
                 base::Unretained(this)));
}

void DeviceOffHoursController::StopOffHoursTimer() {
  timer_->Stop();
}

void DeviceOffHoursController::NotifyOffHoursModeNeedsUpdate() const {
  VLOG(1) << "OffHours mode is probably changed.";
  for (auto& observer : observers_)
    observer.OnOffHoursModeNeedsUpdate();
}

void DeviceOffHoursController::OnNetworkChanged(
    net::NetworkChangeNotifier::ConnectionType type) {
  NotifyOffHoursModeNeedsUpdate();
}

void DeviceOffHoursController::UpdateOffHoursMode(
    const em::ChromeDeviceSettingsProto& input_policies) {
  std::vector<Interval> intervals =
      GetIntervalsIfExists(input_policies, clock_.get());
  base::Time::Exploded exploded;
  clock_->Now().UTCExplode(&exploded);
  int day_of_week = exploded.day_of_week;
  if (day_of_week == 0)
    day_of_week = 7;
  WeeklyTime current_time(day_of_week,
                          exploded.hour * kHour.InMilliseconds() +
                              exploded.minute * kMinute.InMilliseconds() +
                              exploded.second * 1000);
  base::TimeDelta till_off_hours = kWeek;
  for (const auto& interval : intervals) {
    if (WeeklyTime::GetDuration(interval.getStart(), interval.getEnd())
            .is_zero())
      continue;
    if (interval.Contains(current_time)) {
      SetOffHoursMode(true);
      StartOffHoursTimer(
          WeeklyTime::GetDuration(current_time, interval.getEnd()));
      return;
    }
    till_off_hours =
        std::min(till_off_hours,
                 WeeklyTime::GetDuration(current_time, interval.getStart()));
  }
  if (till_off_hours < kWeek)
    StartOffHoursTimer(till_off_hours);
  SetOffHoursMode(false);
}

}  // namespace policy
