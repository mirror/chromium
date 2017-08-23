// Copyright (c) 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/chromeos/policy/device_off_hours_controller.h"

#include <utility>

#include "base/memory/ptr_util.h"
#include "base/time/time.h"

namespace em = enterprise_management;

namespace policy {
namespace {

const int kMillisecondsInDay = base::TimeDelta::FromDays(1).InMilliseconds();

// WeeklyTime class contains weekday (1..7) anf time in milliseconds.
class WeeklyTime {
 public:
  int weekday;
  // Time of day in milliseconds from the beginning of the day.
  int milliseconds;

  WeeklyTime() {}

  WeeklyTime(int w, int ms) : weekday(w), milliseconds(ms) {}

  std::unique_ptr<base::DictionaryValue> ToValue() {
    auto weekly_time = base::MakeUnique<base::DictionaryValue>();
    weekly_time->SetInteger("weekday", weekday);
    weekly_time->SetInteger("time", milliseconds);
    return weekly_time;
  }
};

// Time interval struct, interval = [start, end]
class Interval {
 public:
  WeeklyTime start;
  WeeklyTime end;

  Interval() {}

  Interval(WeeklyTime st, WeeklyTime en) : start(st), end(en) {}

  std::unique_ptr<base::DictionaryValue> ToValue() {
    auto interval = base::MakeUnique<base::DictionaryValue>();
    interval->SetDictionary("start", start.ToValue());
    interval->SetDictionary("end", end.ToValue());
    return interval;
  }
};

// Get WeeklyTime structure from WeeklyTimeProto and put it to |weekly_time|
// Return false if |weekly_time| won't correct
bool GetWeeklyTime(WeeklyTime* weekly_time,
                   const em::WeeklyTimeProto& container) {
  if (!container.has_weekday() ||
      container.weekday() == em::WeeklyTimeProto::DAY_OF_WEEK_UNSPECIFIED) {
    LOG(ERROR) << "Day of week in interval is absent or unspecified.";
    return false;
  }
  if (!container.has_time()) {
    LOG(ERROR) << "Time in interval is absent.";
    return false;
  }
  int time_of_day = container.time();
  if (!(time_of_day >= 0 && time_of_day < kMillisecondsInDay)) {
    LOG(ERROR) << "Invalid time value: " << time_of_day
               << ", the value should be in [0; " << kMillisecondsInDay << ").";
    return false;
  }
  *weekly_time = WeeklyTime(container.weekday(), time_of_day);
  return true;
}

// Get and return list of time intervals from DeviceOffHoursProto structure
std::vector<std::unique_ptr<Interval>> GetIntervals(
    const em::DeviceOffHoursProto& container) {
  std::vector<std::unique_ptr<Interval>> intervals;
  if (container.interval_size() == 0)
    return intervals;
  for (const auto& entry : container.interval()) {
    if (!entry.has_start() || !entry.has_end())
      continue;
    Interval interval;
    if (GetWeeklyTime(&interval.start, entry.start()) &&
        GetWeeklyTime(&interval.end, entry.end())) {
      intervals.push_back(base::MakeUnique<Interval>(interval));
    }
  }
  return intervals;
}

std::set<std::string> GetIgnoredPolicies(
    const em::DeviceOffHoursProto& container) {
  std::set<std::string> ignored_policies;
  if (container.ignored_policy_size() == 0)
    return ignored_policies;
  for (const auto& entry : container.ignored_policy()) {
    ignored_policies.insert(entry);
  }
  return ignored_policies;
}

}  // namespace

// static
std::unique_ptr<base::DictionaryValue> DeviceOffHoursController::ConvertToValue(
    const em::DeviceOffHoursProto& container) {
  if (!container.has_timezone())
    return nullptr;
  auto off_hours = base::MakeUnique<base::DictionaryValue>();
  off_hours->SetString("timezone", container.timezone());
  std::vector<std::unique_ptr<Interval>> intervals = GetIntervals(container);
  auto intervals_value = base::MakeUnique<base::ListValue>();
  for (size_t i = 0; i < intervals.size(); i++) {
    intervals_value->Append(intervals[i]->ToValue());
  }
  off_hours->SetList("intervals", std::move(intervals_value));
  std::set<std::string> ignored_policies = GetIgnoredPolicies(container);
  auto ignored_policies_value = base::MakeUnique<base::ListValue>();
  for (const auto& policy : ignored_policies) {
    ignored_policies_value->AppendString(policy);
  }
  off_hours->SetList("ignored_policies", std::move(ignored_policies_value));
  return off_hours;
}

}  // namespace policy
