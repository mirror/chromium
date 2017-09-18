// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/chromeos/policy/device_off_hours_controller.h"

#include <string>
#include <utility>
#include <vector>

#include "base/memory/ptr_util.h"
#include "base/strings/utf_string_conversions.h"
#include "base/time/time.h"
#include "chrome/browser/chromeos/policy/device_policy_remover.h"
#include "chrome/browser/chromeos/policy/off_hours/weekly_time.h"
#include "chrome/browser/chromeos/settings/cros_settings.h"
#include "chrome/browser/chromeos/settings/device_settings_service.h"
#include "third_party/icu/source/i18n/unicode/gregocal.h"

namespace em = enterprise_management;

namespace policy {
namespace {

constexpr base::TimeDelta kMinute = base::TimeDelta::FromMinutes(1);
constexpr base::TimeDelta kHour = base::TimeDelta::FromHours(1);
constexpr base::TimeDelta kDay = base::TimeDelta::FromDays(1);
constexpr base::TimeDelta kWeek = base::TimeDelta::FromDays(7);

// Put time in milliseconds which is added to GMT to get local time to |offset|
// considering current daylight time. Return true if there was no error.
bool GetTimezoneOffset(const std::string& timezone, int* offset) {
  auto zone = base::WrapUnique(
      icu::TimeZone::createTimeZone(icu::UnicodeString::fromUTF8(timezone)));
  if (*zone == icu::TimeZone::getUnknown()) {
    LOG(ERROR) << "Unsupported timezone: " << timezone;
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

// Convert input day of week and time to WeeklyTime structure in GMT timezone
// considering daylight time. |gmt_offset| is time in milliseconds which is
// added to GMT to get input time.
WeeklyTime ConvertToUTCWeeklyTime(int day_of_week,
                                  int milliseconds,
                                  int gmt_offset) {
  // Convert time in milliseconds to GMT time considering day offset.
  int time_in_gmt = milliseconds - gmt_offset;
  // Make |time_in_gmt| positive number (add number of milliseconds per week)
  // for easier evaluation.
  time_in_gmt += kWeek.InMilliseconds();
  // Get milliseconds from the start of the day.
  int milliseconds_in_gmt = time_in_gmt % kDay.InMilliseconds();
  int day_offset = time_in_gmt / kDay.InMilliseconds();
  // Convert day of week to GMT timezone considering week is cyclic. +/- 1 is
  // because day of week is from 1 to 7.
  int day_of_week_in_gmt = (day_of_week + day_offset - 1) % 7 + 1;
  return WeeklyTime(day_of_week_in_gmt, milliseconds_in_gmt);
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
  return base::MakeUnique<WeeklyTime>(weekly_time.day_of_week(),
                                      weekly_time.milliseconds());
}

// Get and return list of time intervals from DeviceOffHoursProto structure.
// If |convert_to_UTC| is true then intervals convert to UTC timezone.
std::vector<Interval> GetIntervals(const em::DeviceOffHoursProto& container,
                                   bool convert_to_UTC) {
  std::vector<Interval> intervals;
  int gmt_offset = 0;
  if (convert_to_UTC) {
    bool no_offset_error = GetTimezoneOffset(container.timezone(), &gmt_offset);
    if (!no_offset_error)
      return intervals;
  }
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

// If |input_policies| contains DeviceOffHoursProto then return list of
// "OffHours" intervals else return empty list.
std::vector<Interval> GetIntervalsIfExists(
    const em::ChromeDeviceSettingsProto& input_policies) {
  if (!input_policies.has_device_off_hours()) {
    return {};
  }
  const em::DeviceOffHoursProto& container(input_policies.device_off_hours());
  if (!container.has_timezone()) {
    return {};
  }
  return GetIntervals(container, true);
}

}  // namespace

std::unique_ptr<base::DictionaryValue> ConvertOffHoursProtoToValue(
    const em::DeviceOffHoursProto& container) {
  if (!container.has_timezone())
    return nullptr;
  auto off_hours = base::MakeUnique<base::DictionaryValue>();
  off_hours->SetString("timezone", container.timezone());
  std::vector<Interval> intervals = GetIntervals(container, false);
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

DeviceOffHoursController::DeviceOffHoursController() {
  off_hours_intervals_ = {};
  off_hours_mode_ = false;
  // TODO(yakovleva): Find observer which notifies that device wakes up
  // including time when device is offline.
}

DeviceOffHoursController::~DeviceOffHoursController() {
  StopOffHoursTimer();
}

void DeviceOffHoursController::UpdateOffHoursMode() {
  if (off_hours_intervals_.empty()) {
    StopOffHoursTimer();
    SetOffHoursMode(false);
    return;
  }
  base::Time::Exploded exploded;
  base::Time::Now().UTCExplode(&exploded);
  int day_of_week = exploded.day_of_week;
  // Exploded contains 0-based day of week (0 = Sunday, etc.)
  if (day_of_week == 0)
    day_of_week = 7;
  WeeklyTime current_time(day_of_week,
                          exploded.hour * kHour.InMilliseconds() +
                              exploded.minute * kMinute.InMilliseconds() +
                              exploded.second * 1000);
  // "OffHours" intervals repeat every week, therefore the maximum duration till
  // next "OffHours" interval is one week.
  base::TimeDelta till_off_hours = kWeek;
  for (const auto& interval : off_hours_intervals_) {
    if (WeeklyTime::GetDuration(interval.start(), interval.end()).is_zero())
      continue;
    if (interval.Contains(current_time)) {
      SetOffHoursMode(true);
      StartOffHoursTimer(WeeklyTime::GetDuration(current_time, interval.end()));
      return;
    }
    till_off_hours =
        std::min(till_off_hours,
                 WeeklyTime::GetDuration(current_time, interval.start()));
  }
  StartOffHoursTimer(till_off_hours);
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
  DCHECK_GT(delay.InMilliseconds(), 0);
  DVLOG(1) << "OffHours mode timer starts for "
           << delay.InMilliseconds() / kMinute.InMilliseconds() << " minutes.";
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
  off_hours_intervals_ = GetIntervalsIfExists(device_settings_proto);
  UpdateOffHoursMode();
}

}  // namespace policy
