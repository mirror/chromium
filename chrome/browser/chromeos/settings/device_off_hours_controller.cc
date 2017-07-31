// Copyright (c) 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/chromeos/settings/device_off_hours_controller.h"

#include <utility>

namespace em = enterprise_management;

namespace chromeos {

// using google::protobuf::Descriptor;
// using google::protobuf::FieldDescriptor;

namespace {

// WeeklyTime struct, contains weekday (1..7), hours and minutes (24h)
struct WeeklyTime {
  int weekday;
  int hours;
  int minutes;
  WeeklyTime() {}
  WeeklyTime(int w, int h, int m) : weekday(w), hours(h), minutes(m) {}
  std::string to_string() const {
    return "weekday: " + std::to_string(weekday) +
           ", hours: " + std::to_string(hours) +
           ", minutes: " + std::to_string(minutes);
  }
};

// Time interval struct, interval = [start, end]
struct Interval {
  WeeklyTime start;
  WeeklyTime end;
  Interval() {}
  Interval(WeeklyTime st, WeeklyTime en) : start(st), end(en) {}
  std::string to_string() const {
    return "[" + start.to_string() + "; " + end.to_string() + "]";
  }
};

// Compare two WeeklyTimes
int cmp_WeeklyTimes(const WeeklyTime* w1, const WeeklyTime* w2) {
  if (w1->weekday == w2->weekday && w1->hours == w2->hours &&
      w1->minutes == w2->minutes)
    return 0;
  if ((w1->weekday < w2->weekday) ||
      (w1->weekday == w2->weekday && w1->hours < w2->hours) ||
      (w1->weekday == w2->weekday && w1->hours == w2->hours &&
       w1->minutes < w2->minutes))
    return -1;
  return 1;
}

// Check if WeeklyTime |w| in interval |interval|
bool in_interval(const Interval* interval, const WeeklyTime* w) {
  if (cmp_WeeklyTimes(&interval->start, &interval->end) == 0) {
    if (cmp_WeeklyTimes(&interval->start, w) == 0) {
      return true;
    }
  } else if (cmp_WeeklyTimes(&interval->start, &interval->end) < 0) {
    if (cmp_WeeklyTimes(w, &interval->start) >= 0 &&
        cmp_WeeklyTimes(w, &interval->end) <= 0) {
      return true;
    }
  } else {
    if (cmp_WeeklyTimes(w, &interval->start) >= 0 ||
        cmp_WeeklyTimes(w, &interval->end) <= 0) {
      return true;
    }
  }
  return false;
}

// Get WeeklyTime structure from WeeklyTimeProto and put it to |weekly_time|
// Return false if |weekly_time| won't correct
bool get_WeeklyTime(WeeklyTime* weekly_time,
                    const em::WeeklyTimeProto& container) {
  LOG(ERROR) << "Daria parse weekly time";
  bool is_valid = true;
  if (container.has_weekday() && container.weekday().has_day()) {
    weekly_time->weekday = container.weekday().day();
  } else {
    LOG(ERROR) << "Day of week in interval can't be absent.";
    is_valid = false;
  }
  if (container.has_time()) {
    auto time_of_day = container.time();
    if (time_of_day.has_hours()) {
      int hours = time_of_day.hours();
      if (!(hours >= 0 && hours <= 23)) {
        LOG(ERROR) << "Invalid hours value: " << hours
                   << ", the value should be in [0; 23].";
        is_valid = false;
      }
      weekly_time->hours = hours;
    } else {
      LOG(ERROR) << "Hours in interval can't be absent.";
      is_valid = false;
    }
    if (time_of_day.has_minutes()) {
      int minutes = time_of_day.minutes();
      if (!(minutes >= 0 && minutes <= 59)) {
        LOG(ERROR) << "Invalid minutes value: " << minutes
                   << ", the value should be in [0; 59].";
        is_valid = false;
      }
      weekly_time->minutes = minutes;
    } else {
      LOG(ERROR) << "Minutes in interval can't be absent.";
      is_valid = false;
    }
  } else {
    LOG(ERROR) << "Time in interval can't be absent.";
    is_valid = false;
  }
  if (is_valid) {
    LOG(ERROR) << "Daria week time: " << weekly_time->to_string();
    return true;
  }
  return false;
}

// Get and return list of time intervals from DeviceOffHoursProto structure
std::vector<std::unique_ptr<Interval>> get_intervals(
    const em::DeviceOffHoursProto& container) {
  LOG(ERROR) << "Daria get_intervals";
  std::vector<std::unique_ptr<Interval>> intervals;
  for (const auto& entry : container.interval()) {
    Interval interval;
    if (entry.has_start()) {
      if (!get_WeeklyTime(&interval.start, entry.start())) {
        continue;
      }
    }
    if (entry.has_end()) {
      if (!get_WeeklyTime(&interval.end, entry.end())) {
        continue;
      }
    }
    intervals.push_back(base::MakeUnique<Interval>(interval));
  }
  return intervals;
}

// Go through interval and check off_hours mode
bool is_off_hours_mode(
    const std::vector<std::unique_ptr<Interval>>& intervals) {
  time_t theTime = time(NULL);
  struct tm* aTime = localtime(&theTime);
  WeeklyTime cur_t(aTime->tm_wday, aTime->tm_hour, aTime->tm_min);
  LOG(ERROR) << "Daria DATE:"
             << " weekday:" << cur_t.to_string();
  for (size_t i = 0; i < intervals.size(); i++) {
    LOG(ERROR) << "Daria interval " << intervals[i]->to_string();
    if (in_interval(intervals[i].get(), &cur_t)) {
      return true;
    }
  }
  return false;
}

// TODO(yakovleva) unuseless function
// std::vector<std::unique_ptr<Interval>> get_intervals(const base::ListValue*
// input_intervals) {
//    std::vector<std::unique_ptr<Interval>> intervals;
//
//    for (size_t i = 0; i < input_intervals->GetList().size(); i++) {
//      const base::DictionaryValue* cur_interval;
//      input_intervals->GetList()[i].GetAsDictionary(&cur_interval);
//      std::unique_ptr<WeeklyTime> weekly_time;
//      for (const std::string& timestamp : timestamps) {
//        const base::DictionaryValue* input_weektime;
//        cur_interval->GetDictionary(timestamp, &input_weektime);
//        const base::DictionaryValue* cur_time;
//        input_weektime->GetDictionary("time", &cur_time);
//        int weekday;
//        input_weektime->GetInteger("weekday", &weekday);
//        cur_weektime[timestamp].push_back(weekday);
//        int hours;
//        cur_time->GetInteger("hours", &hours);
//        cur_weektime[timestamp].push_back(hours);
//        int minutes;
//        cur_time->GetInteger("minutes", &minutes);
//        cur_weektime[timestamp].push_back(minutes);
//        LOG(ERROR) << "Daria interval " << timestamp << ": " << weekday << ";
//        " << hours << ":" << minutes;
//      }
//      if (in_weektimes(cur_weektime[timestamps[0]],
//      cur_weektime[timestamps[1]], cur_t)) {
//        off_hours_mode = true;
//        break;
//      }
//    }
//}

}  // namespace

// If device_off_hours policy will be set and current time will be
// in off_hours intervals remove off_hours policies from
// ChromeDeviceSettingsProto. This policies will be apply with default values.
std::unique_ptr<em::ChromeDeviceSettingsProto>
DeviceOffHoursController::ApplyOffHoursMode(
    em::ChromeDeviceSettingsProto* input_policies) {
  if (!input_policies->has_device_off_hours()) {
    return nullptr;
  }
  LOG(ERROR) << "Daria OFF HOURS POLICIES";
  const em::DeviceOffHoursProto& container(input_policies->device_off_hours());
  std::vector<std::unique_ptr<Interval>> intervals = get_intervals(container);
  if (!is_off_hours_mode(intervals) || container.policy_size() == 0) {
    return nullptr;
  }
  std::vector<std::string> off_policies;
  for (const auto& p : container.policy()) {
    off_policies.push_back(p);
  }
  em::ChromeDeviceSettingsProto policies;
  policies.CopyFrom(*input_policies);

  //  "policy" : [ "DeviceAllowNewUsers", "DeviceGuestModeEnabled" ],
  policies.clear_allow_new_users();
  policies.clear_guest_mode_enabled();
  return base::MakeUnique<em::ChromeDeviceSettingsProto>(policies);
}

// TODO(yakovleva) unuseless function
void DeviceOffHoursController::ApplyOffHoursMode(policy::PolicyMap* policies) {
  if (!policies->GetValue("DeviceOffHours")) {
    return;
  }
  const base::DictionaryValue* off_hours;
  policies->GetValue("DeviceOffHours")->GetAsDictionary(&off_hours);
  const base::ListValue* off_policies;
  off_hours->GetList("policies", &off_policies);

  LOG(ERROR) << "Daria OFF HOURS POLICIES2";
  for (size_t i = 0; i < off_policies->GetList().size(); i++) {
    LOG(ERROR) << "Daria off policy:" << off_policies->GetList()[i].GetString();
  }

  bool off_hours_mode = true;
  const base::ListValue* intervals;
  off_hours->GetList("intervals", &intervals);

  if (!off_hours_mode) {
    return;
  }
  for (policy::PolicyMap::const_iterator it = policies->begin();
       it != policies->end(); it++) {
    auto policy_name = it->first;
    auto* policy_value = it->second.value.get();
    bool in_off_hours = false;
    if (policy_value->is_bool()) {
      bool val;
      policy_value->GetAsBoolean(&val);
      LOG(ERROR) << "Daria policy: " << policy_name << " " << val;
    }
    for (size_t i = 0; i < off_policies->GetList().size(); i++) {
      if (policy_name == off_policies->GetList()[i].GetString()) {
        in_off_hours = true;
        break;
      }
    }
    if (in_off_hours) {
      // TODO set policy settings
      //      policies->Set(
      //                policy_name, POLICY_LEVEL_MANDATORY,
      //                POLICY_SCOPE_MACHINE, POLICY_SOURCE_CLOUD,
      //                base::MakeUnique<base::Value>(true),
      //                nullptr);
    }
  }
}

}  // namespace chromeos
