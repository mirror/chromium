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
#include "base/time/time.h"
#include "chrome/browser/chromeos/policy/device_policy_remover.h"
#include "chrome/browser/chromeos/settings/cros_settings.h"
#include "chrome/browser/chromeos/settings/device_settings_service.h"
#include "chrome/browser/lifetime/application_lifetime.h"
#include "unicode/gregocal.h"

namespace em = enterprise_management;

namespace policy {
namespace {

constexpr base::TimeDelta kMillisecondsInMinute =
    base::TimeDelta::FromMinutes(1);
constexpr base::TimeDelta kMillisecondsInHour = base::TimeDelta::FromHours(1);
constexpr base::TimeDelta kMillisecondsInDay = base::TimeDelta::FromDays(1);
constexpr base::TimeDelta kMillisecondsInWeek = base::TimeDelta::FromDays(7);

// WeeklyTime class contains day of week and time.
// Day of week is number from 1 to 7 (1 = Monday, 2 = Tuesday, etc.)
// Time is in milliseconds from the beginning of the day.
struct WeeklyTime {
  WeeklyTime(int day_of_week, int milliseconds)
      : day_of_week(day_of_week), milliseconds(milliseconds) {
    DCHECK_GT(day_of_week, 0);
    DCHECK_LE(day_of_week, 7);
    DCHECK_GE(milliseconds, 0);
    DCHECK_LT(milliseconds, kMillisecondsInDay.InMilliseconds());
  }

  std::unique_ptr<base::DictionaryValue> ToValue() const {
    auto weekly_time = base::MakeUnique<base::DictionaryValue>();
    weekly_time->SetInteger("day_of_week", day_of_week);
    weekly_time->SetInteger("time", milliseconds);
    return weekly_time;
  }

  // Return true if current week time is earlier than |other|
  // when it happens in the same week.
  bool EarlierInWeek(const WeeklyTime& other) const {
    return std::tie(day_of_week, milliseconds) <=
           std::tie(other.day_of_week, other.milliseconds);
  }

  // Number of weekday (1 = Monday, 2 = Tuesday, etc.)
  int day_of_week;

  // Time of day in milliseconds from the beginning of the day.
  int milliseconds;
};

// Time interval struct, interval = [start, end).
struct Interval {
  Interval(const WeeklyTime& start, const WeeklyTime& end)
      : start(start), end(end) {}

  std::unique_ptr<base::DictionaryValue> ToValue() const {
    auto interval = base::MakeUnique<base::DictionaryValue>();
    interval->SetDictionary("start", start.ToValue());
    interval->SetDictionary("end", end.ToValue());
    return interval;
  }

  WeeklyTime start;
  WeeklyTime end;
};

// Time duration in milliseconds from |start| till |end| week times.
// |end| time is always after |start| time.
// It's possible because week time is cyclically.
// (i.e. [Friday 17:00, Monday 9:00) )
int GetDuration(const WeeklyTime& start, const WeeklyTime& end) {
  if (start.EarlierInWeek(end))
    return (end.day_of_week - start.day_of_week) *
               kMillisecondsInDay.InMilliseconds() +
           end.milliseconds - start.milliseconds;
  return kMillisecondsInWeek.InMilliseconds() - GetDuration(end, start);
}

// Check if |w| is in [interval.start, interval.end).
// |end| time is always after |start| time.
// It's possible because week time is cyclically.
// (i.e. [Friday 17:00, Monday 9:00) )
bool IntervalCoversWeeklyTime(const Interval& interval, const WeeklyTime& w) {
  int interval_duration = GetDuration(interval.start, interval.end);
  if (interval_duration == 0)
    return false;
  if (GetDuration(interval.start, w) + GetDuration(w, interval.end) ==
      interval_duration)
    return true;
  return false;
}

// Put time in milliseconds which is added to GMT to get local time
// to |offset| considering current daylight time.
// Return true if there was no errors.
bool GetTimezoneOffset(const std::string& timezone, int* offset) {
  auto zone = base::WrapUnique(
      icu::TimeZone::createTimeZone(icu::UnicodeString::fromUTF8(timezone)));
  if (*zone == icu::TimeZone::getUnknown()) {
    LOG(ERROR) << "Timezone error";
    return false;
  }
  // Time in milliseconds which is added to GMT to get local time.
  int gmt_offset = zone->getRawOffset();
  // Time in milliseconds which is added to local standard time
  // to get local wall clock time.
  int dst_offset = zone->getDSTSavings();
  UErrorCode status = U_ZERO_ERROR;
  std::unique_ptr<icu::GregorianCalendar> gregorian_calendar =
      base::MakeUnique<icu::GregorianCalendar>(*zone, status);
  if (U_FAILURE(status)) {
    LOG(ERROR) << "Gregorian calendar error = " << u_errorName(status);
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

// Convert input day of week and time in |timezone|
// to WeeklyTime structure in GMT timezone
// considering daylight time.
std::unique_ptr<WeeklyTime> ConvertToUTCWeeklyTime(int day_of_week,
                                                   int milliseconds,
                                                   int gmt_offset) {
  // Convert gmt_offset to time in milliseconds which is added to local time to
  // get GMT. Make it positive number (add number of milliseconds per week) for
  // easier evaluation.
  gmt_offset = kMillisecondsInWeek.InMilliseconds() - gmt_offset;
  // Convert time in milliseconds to GMT timezone considering day offset.
  int milliseconds_in_gmt =
      (milliseconds + gmt_offset) % kMillisecondsInDay.InMilliseconds();
  int day_offset =
      (milliseconds + gmt_offset) / kMillisecondsInDay.InMilliseconds();
  // Convert day of week to GMT timezone considering week cyclicity.
  // +/- is because day of week is from 1 to 7.
  int day_of_week_in_gmt = (day_of_week + day_offset - 1) % 7 + 1;
  return base::MakeUnique<WeeklyTime>(day_of_week_in_gmt, milliseconds_in_gmt);
}

// Get and return WeeklyTime structure from WeeklyTimeProto
// Return nullptr if WeeklyTime structure isn't correct.
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
  if (!(time_of_day >= 0 &&
        time_of_day < kMillisecondsInDay.InMilliseconds())) {
    LOG(ERROR) << "Invalid time value: " << time_of_day
               << ", the value should be in [0; "
               << kMillisecondsInDay.InMilliseconds() << ").";
    return nullptr;
  }
  return ConvertToUTCWeeklyTime(container.day_of_week(), time_of_day,
                                gmt_offset);
}

// Get and return list of time intervals from DeviceOffHoursProto structure.
std::vector<Interval> GetIntervals(const em::DeviceOffHoursProto& container,
                                   const std::string& timezone = "UTC") {
  std::vector<Interval> intervals;
  int gmt_offset;
  bool is_offset_error = GetTimezoneOffset(timezone, &gmt_offset);
  if (!is_offset_error)
    return intervals;
  for (const auto& entry : container.intervals()) {
    if (!entry.has_start() || !entry.has_end()) {
      LOG(WARNING) << "Skipping interval without start or/and end.";
      continue;
    }
    auto start = GetWeeklyTime(entry.start(), gmt_offset);
    auto end = GetWeeklyTime(entry.end(), gmt_offset);
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

}  // namespace

std::unique_ptr<base::DictionaryValue> ConvertOffHoursProtoToValue(
    const em::DeviceOffHoursProto& container) {
  if (!container.has_timezone())
    return nullptr;
  auto off_hours = base::MakeUnique<base::DictionaryValue>();
  off_hours->SetString("timezone", container.timezone());
  std::vector<Interval> intervals = GetIntervals(container);
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

std::unique_ptr<em::ChromeDeviceSettingsProto> GetOffHoursProto(
    const em::ChromeDeviceSettingsProto& input_policies) {
  const em::DeviceOffHoursProto& container(input_policies.device_off_hours());
  std::vector<std::string> ignored_policies = GetIgnoredPolicies(container);
  std::unique_ptr<em::ChromeDeviceSettingsProto> policies =
      base::MakeUnique<em::ChromeDeviceSettingsProto>(input_policies);
  RemovePolicies(policies.get(), ignored_policies);
  return policies;
}

// Observer
DeviceOffHoursController::Observer::~Observer() {}

void DeviceOffHoursController::Observer::OnOffHoursModeMaybeChanged() {}

void DeviceOffHoursController::AddObserver(Observer* observer) {
  observers_.AddObserver(observer);
}

void DeviceOffHoursController::RemoveObserver(Observer* observer) {
  observers_.RemoveObserver(observer);
}

DeviceOffHoursController::DeviceOffHoursController() {
  off_hours_mode_ = false;
  net::NetworkChangeNotifier::AddNetworkChangeObserver(this);
}

DeviceOffHoursController::~DeviceOffHoursController() {
  net::NetworkChangeNotifier::RemoveNetworkChangeObserver(this);
  StopOffHoursTimer();
}

bool DeviceOffHoursController::IsOffHoursMode() {
  return off_hours_mode_;
}

bool DeviceOffHoursController::SetOffHoursMode() {
  if (off_hours_mode_)
    return false;
  VLOG(1) << "OffHours mode is on.";
  off_hours_mode_ = true;
  return true;
}

bool DeviceOffHoursController::UnsetOffHoursMode() {
  if (!off_hours_mode_)
    return false;
  VLOG(1) << "OffHours mode is off.";
  off_hours_mode_ = false;
  return true;
}

void DeviceOffHoursController::StartOffHoursTimer(base::TimeDelta delay) {
  DCHECK_GT(delay.InMilliseconds(), 0);
  if (timer_.IsRunning() && timer_.GetCurrentDelay() < delay)
    return;
  VLOG(1) << "OffHours mode timer starts for "
          << delay.InMilliseconds() / kMillisecondsInMinute.InMilliseconds()
          << " minutes.";
  timer_.Start(FROM_HERE, delay,
               base::Bind(&DeviceOffHoursController::NotifyOffHoursModeChanged,
                          base::Unretained(this)));
}

void DeviceOffHoursController::StopOffHoursTimer() {
  timer_.Stop();
}

void DeviceOffHoursController::NotifyOffHoursModeChanged() const {
  VLOG(1) << "OffHours mode is probably changed.";
  for (auto& observer : observers_)
    observer.OnOffHoursModeMaybeChanged();
}

void DeviceOffHoursController::OnNetworkChanged(
    net::NetworkChangeNotifier::ConnectionType type) {
  NotifyOffHoursModeChanged();
}

// If |input_policies| contains DeviceOffHoursProto
// then return list of "OffHours" intervals
// else return empty list.
std::vector<Interval> GetIntervalsIfExists(
    const em::ChromeDeviceSettingsProto& input_policies) {
  if (!input_policies.has_device_off_hours()) {
    return {};
  }
  const em::DeviceOffHoursProto& container(input_policies.device_off_hours());
  if (!container.has_timezone()) {
    return {};
  }
  return GetIntervals(container, container.timezone());
}

void DeviceOffHoursController::UpdateOffHoursMode(
    const em::ChromeDeviceSettingsProto& input_policies) {
  // TODO(yakovleva) log out user if user isn't allowed after off hours mode.
  std::vector<Interval> intervals = GetIntervalsIfExists(input_policies);
  base::Time::Exploded exploded;
  base::Time::Now().UTCExplode(&exploded);
  WeeklyTime cur_t(
      exploded.day_of_week,
      exploded.hour * kMillisecondsInHour.InMilliseconds() +
          exploded.minute * kMillisecondsInMinute.InMilliseconds() +
          exploded.second * 1000);
  int before_off_hours = kMillisecondsInWeek.InMilliseconds();
  for (const auto& interval : intervals) {
    if (IntervalCoversWeeklyTime(interval, cur_t)) {
      SetOffHoursMode();
      StartOffHoursTimer(
          base::TimeDelta::FromMilliseconds(GetDuration(cur_t, interval.end)));
      return;
    }
    before_off_hours =
        std::min(before_off_hours, GetDuration(cur_t, interval.start));
  }
  if (before_off_hours < kMillisecondsInWeek.InMilliseconds())
    StartOffHoursTimer(base::TimeDelta::FromMilliseconds(before_off_hours));
  if (UnsetOffHoursMode())
    chrome::AttemptUserExit();
}

}  // namespace policy
