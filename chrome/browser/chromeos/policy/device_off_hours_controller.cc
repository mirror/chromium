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
#include "third_party/icu/source/i18n/unicode/gregocal.h"

namespace em = enterprise_management;

namespace policy {
namespace {

constexpr base::TimeDelta kMinute = base::TimeDelta::FromMinutes(1);
constexpr base::TimeDelta kHour = base::TimeDelta::FromHours(1);
constexpr base::TimeDelta kDay = base::TimeDelta::FromDays(1);
constexpr base::TimeDelta kWeek = base::TimeDelta::FromDays(7);

// WeeklyTime class contains day of week and time. Day of week is number from 1
// to 7 (1 = Monday, 2 = Tuesday, etc.) Time is in milliseconds from the
// beginning of the day.
class WeeklyTime {
 public:
  WeeklyTime(int day_of_week, int milliseconds)
      : day_of_week_(day_of_week), milliseconds_(milliseconds) {
    DCHECK_GT(day_of_week, 0);
    DCHECK_LE(day_of_week, 7);
    DCHECK_GE(milliseconds, 0);
    DCHECK_LT(milliseconds, kDay.InMilliseconds());
  }

  std::unique_ptr<base::DictionaryValue> ToValue() const {
    auto weekly_time = base::MakeUnique<base::DictionaryValue>();
    weekly_time->SetInteger("day_of_week", day_of_week_);
    weekly_time->SetInteger("time", milliseconds_);
    return weekly_time;
  }

  int getDayOfWeek() const { return day_of_week_; }

  int getMilliseconds() const { return milliseconds_; }

  // Return duration from |start| till |end| week times. |end| time
  // is always after |start| time. It's possible because week time is cyclic.
  // (i.e. [Friday 17:00, Monday 9:00) )
  static base::TimeDelta GetDuration(const WeeklyTime& start,
                                     const WeeklyTime& end) {
    int duration =
        (end.getDayOfWeek() - start.getDayOfWeek()) * kDay.InMilliseconds() +
        end.getMilliseconds() - start.getMilliseconds();
    if (duration < 0)
      duration += kWeek.InMilliseconds();
    return base::TimeDelta::FromMilliseconds(duration);
  }

 private:
  // Number of weekday (1 = Monday, 2 = Tuesday, etc.)
  int day_of_week_;

  // Time of day in milliseconds from the beginning of the day.
  int milliseconds_;
};

// Time interval struct, interval = [start, end).
class Interval {
 public:
  Interval(const WeeklyTime& start, const WeeklyTime& end)
      : start_(start), end_(end) {}

  std::unique_ptr<base::DictionaryValue> ToValue() const {
    auto interval = base::MakeUnique<base::DictionaryValue>();
    interval->SetDictionary("start", start_.ToValue());
    interval->SetDictionary("end", end_.ToValue());
    return interval;
  }

  // Check if |w| is in [interval.start, interval.end). |end| time is always
  // after |start| time. It's possible because week time is cyclically. (i.e.
  // [Friday 17:00, Monday 9:00) )
  bool Contains(const WeeklyTime& w) const {
    base::TimeDelta interval_duration = WeeklyTime::GetDuration(start_, end_);
    if (interval_duration.is_zero())
      return false;
    return WeeklyTime::GetDuration(start_, w) +
               WeeklyTime::GetDuration(w, end_) ==
           interval_duration;
  }

  WeeklyTime getStart() const { return start_; }

  WeeklyTime getEnd() const { return end_; }

 private:
  WeeklyTime start_;
  WeeklyTime end_;
};

// Put time in milliseconds which is added to GMT to get local time to |offset|
// considering current daylight time. Return true if there was no error.
bool GetTimezoneOffset(const std::string& timezone, int* offset) {
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

// Convert input day of week and time in |timezone| to WeeklyTime structure in
// GMT timezone considering daylight time.
// |gmt_offset| is time in milliseconds which is added to GMT to get input time.
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
  return std::unique_ptr<WeeklyTime>(&weekly_time);
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

bool DeviceOffHoursController::SetOffHoursMode(bool off_hours_enabled) {
  if (off_hours_mode_ == off_hours_enabled)
    return false;
  off_hours_mode_ = off_hours_enabled;
  if (off_hours_mode_)
    VLOG(1) << "OffHours mode is on.";
  else
    VLOG(1) << "OffHours mode is off.";
  return true;
}

void DeviceOffHoursController::StartOffHoursTimer(base::TimeDelta delay) {
  DCHECK_GT(delay.InMilliseconds(), 0);
  if (timer_.IsRunning() && timer_.GetCurrentDelay() < delay)
    return;
  VLOG(1) << "OffHours mode timer starts for "
          << delay.InMilliseconds() / kMinute.InMilliseconds() << " minutes.";
  timer_.Start(
      FROM_HERE, delay,
      base::Bind(&DeviceOffHoursController::NotifyOffHoursModeNeedsUpdate,
                 base::Unretained(this)));
}

void DeviceOffHoursController::StopOffHoursTimer() {
  timer_.Stop();
}

void DeviceOffHoursController::NotifyOffHoursModeNeedsUpdate() const {
  VLOG(1) << "OffHours mode is probably changed.";
  for (auto& observer : observers_)
    observer.OnOffHoursModeNeedsUpdate();
}

void DeviceOffHoursController::OnNetworkChanged(
    net::NetworkChangeNotifier::ConnectionType type) {
  // Triggered when the device wakes up.
  NotifyOffHoursModeNeedsUpdate();
}

void DeviceOffHoursController::UpdateOffHoursMode(
    const em::ChromeDeviceSettingsProto& input_policies) {
  // TODO(yakovleva): Log out user only if user isn't allowed after "OffHours"
  // mode.
  std::vector<Interval> intervals = GetIntervalsIfExists(input_policies);
  base::Time::Exploded exploded;
  base::Time::Now().UTCExplode(&exploded);
  WeeklyTime current_time(exploded.day_of_week,
                          exploded.hour * kHour.InMilliseconds() +
                              exploded.minute * kMinute.InMilliseconds() +
                              exploded.second * 1000);
  base::TimeDelta till_off_hours = kWeek;
  for (const auto& interval : intervals) {
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
  if (SetOffHoursMode(false))
    chrome::AttemptUserExit();
}

}  // namespace policy
