// Copyright (c) 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/chromeos/policy/device_off_hours_controller.h"

#include <thread>
#include <utility>

namespace em = enterprise_management;

namespace policy {

namespace {

const int kMillisecondsInHour = base::TimeDelta::FromHours(1).InMilliseconds();
const int kMillisecondsInDay = base::TimeDelta::FromDays(1).InMilliseconds();
const int kMillisecondsInWeek = base::TimeDelta::FromDays(7).InMilliseconds();

// WeeklyTime struct, contains weekday (1..7), hours and minutes (24h)
class WeeklyTime {
 public:
  int weekday;
  // Time of day in milliseconds from beginning of the day
  int milliseconds;
  WeeklyTime() {}
  WeeklyTime(int w, int ms) : weekday(w), milliseconds(ms) {}
  WeeklyTime(int w, int ms, std::string timezone) {
    icu::TimeZone* zone =
        icu::TimeZone::createTimeZone(icu::UnicodeString::fromUTF8(timezone));
    // delta time in milliseconds
    int deltaMS = -(zone->getRawOffset());
    // UErrorCode status;
    // icu::GregorianCalendar* gc = new icu::GregorianCalendar(zone, status);
    UBool day_light = false;  // gc->inDaylightTime(status);
    if (day_light) {
      LOG(ERROR) << "Daria: Day Light Time!! timezone = " << timezone;
      deltaMS -= kMillisecondsInHour;
    }
    deltaMS = kMillisecondsInWeek + deltaMS;
    milliseconds = (ms + deltaMS) % kMillisecondsInDay;
    int deltaW = (ms + deltaMS) / kMillisecondsInDay;
    weekday = (w + deltaW - 1) % 7 + 1;
  }
  std::string to_string() const {
    return "weekday: " + std::to_string(weekday) + ", " +
           std::to_string(milliseconds / kMillisecondsInHour) + ":" +
           std::to_string((milliseconds % kMillisecondsInHour) / 1000 / 60);
  }
  bool operator==(const WeeklyTime& wt) const {
    return (weekday == wt.weekday) && (milliseconds == wt.milliseconds);
  }

  bool operator<(const WeeklyTime& wt) const {
    return (weekday < wt.weekday) ||
           (weekday == wt.weekday && milliseconds < wt.milliseconds);
  }
  bool operator<=(const WeeklyTime& wt) const {
    return *this == wt || *this < wt;
  }
  bool operator>(const WeeklyTime& wt) const {
    return !(*this == wt || *this < wt);
  }
  bool operator>=(const WeeklyTime& wt) const {
    return *this == wt || *this > wt;
  }
  // in milliseconds
  int operator-(const WeeklyTime& wt) const {
    int delta_m = 0;
    if (*this > wt) {
      delta_m += (weekday - wt.weekday - 1) * kMillisecondsInDay;  // days
      delta_m += milliseconds +
                 (kMillisecondsInDay - wt.milliseconds);  // milliseconds
      return delta_m;
    }
    return kMillisecondsInWeek - (wt - *this);
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

// Check if WeeklyTime |w| in interval |interval|
bool in_interval(const Interval* interval, const WeeklyTime* w) {
  if (interval->start == interval->end) {
    if (interval->start == *w) {
      return true;
    }
  } else if (interval->start < interval->end) {
    if (*w >= interval->start && *w <= interval->end) {
      return true;
    }
  } else {
    if (*w >= interval->start || *w <= interval->end) {
      return true;
    }
  }
  return false;
}

// Get WeeklyTime structure from WeeklyTimeProto and put it to |weekly_time|
// Return false if |weekly_time| won't correct
bool get_WeeklyTime(WeeklyTime* weekly_time,
                    const em::WeeklyTimeProto& container,
                    const std::string& timezone) {
  LOG(ERROR) << "Daria: parse weekly time";
  if (!container.has_weekday()) {
    LOG(ERROR) << "Day of week in interval can't be absent.";
    return false;
  }
  if (!container.has_time()) {
    LOG(ERROR) << "Time in interval can't be absent.";
    return false;
  }
  int time_of_day = container.time();
  if (!(time_of_day >= 0 && time_of_day < kMillisecondsInDay)) {
    LOG(ERROR) << "Invalid time value: " << time_of_day
               << ", the value should be in [0; " << kMillisecondsInDay << ").";
    return false;
  }
  *weekly_time = WeeklyTime(container.weekday(), time_of_day, timezone);
  LOG(ERROR) << "Daria UTC time: " << weekly_time->to_string();
  return true;
}

// Get and return list of time intervals from DeviceOffHoursProto structure
std::vector<std::unique_ptr<Interval>> get_intervals(
    const em::DeviceOffHoursProto& container) {
  LOG(ERROR) << "Daria get_intervals";
  std::string timezone = "GMT";
  if (container.has_timezone()) {
    timezone = container.timezone();
  }
  std::vector<std::unique_ptr<Interval>> intervals;
  if (container.interval_size() == 0) {
    return intervals;
  }
  for (const auto& entry : container.interval()) {
    Interval interval;
    if (entry.has_start() && entry.has_end()) {
      if (get_WeeklyTime(&interval.start, entry.start(), timezone) &&
          get_WeeklyTime(&interval.end, entry.end(), timezone)) {
        intervals.push_back(base::MakeUnique<Interval>(interval));
      }
    }
  }
  return intervals;
}

// Go through interval and check off_hours mode
bool is_off_hours_mode(
    DeviceOffHoursController* g_device_off_hours_controller,
    const std::vector<std::unique_ptr<Interval>>& intervals) {
  const time_t theTime = base::Time::Now().ToTimeT();
  struct tm* aTime = gmtime(&theTime);
  // TODO(yakovleva) find better convertation
  WeeklyTime cur_t(aTime->tm_wday,
                   (aTime->tm_hour * 60 + aTime->tm_min) * 60 * 1000);
  LOG(ERROR) << "Daria CURRENT TIME UTC:" << cur_t.to_string();
  int before_off_hours = kMillisecondsInWeek;
  for (size_t i = 0; i < intervals.size(); i++) {
    LOG(ERROR) << "Daria interval " << intervals[i]->to_string();
    if (in_interval(intervals[i].get(), &cur_t)) {
      g_device_off_hours_controller->StartOffHoursTimer(intervals[i]->end -
                                                        cur_t + 1);
      return true;
    }
    before_off_hours = std::min(before_off_hours, intervals[i]->start - cur_t);
  }
  if (before_off_hours < kMillisecondsInWeek) {
    g_device_off_hours_controller->StartOffHoursTimer(before_off_hours);
  }
  return false;
}
}  // namespace

static DeviceOffHoursController* g_device_off_hours_controller = NULL;

// Observer
DeviceOffHoursController::Observer::~Observer() {}

void DeviceOffHoursController::Observer::OffHoursModeChanged() {}

void DeviceOffHoursController::AddObserver(Observer* observer) {
  observers_.AddObserver(observer);
  LOG(ERROR) << "Daria "
             << "observer added";
}

void DeviceOffHoursController::RemoveObserver(Observer* observer) {
  observers_.RemoveObserver(observer);
}

// static
void DeviceOffHoursController::Initialize() {
  CHECK(!g_device_off_hours_controller);
  g_device_off_hours_controller = new DeviceOffHoursController();
  LOG(ERROR) << "Daria Initialize OK, thread = " << std::this_thread::get_id();
  LOG(ERROR) << "Daria controller id = " << g_device_off_hours_controller;
}

// static
bool DeviceOffHoursController::IsInitialized() {
  return g_device_off_hours_controller;
}

// static
void DeviceOffHoursController::Shutdown() {
  DCHECK(g_device_off_hours_controller);
  delete g_device_off_hours_controller;
  g_device_off_hours_controller = NULL;
  LOG(ERROR) << "Daria Shutdown OK";
}

// static
DeviceOffHoursController* DeviceOffHoursController::Get() {
  CHECK(g_device_off_hours_controller);
  return g_device_off_hours_controller;
}

DeviceOffHoursController::DeviceOffHoursController() {
  chromeos::system::TimezoneSettings::GetInstance()->AddObserver(this);
  if (chromeos::DBusThreadManager::IsInitialized() &&
      chromeos::DBusThreadManager::Get()->GetSessionManagerClient()) {
    chromeos::DBusThreadManager::Get()->GetSessionManagerClient()->AddObserver(
        this);
  }
}

DeviceOffHoursController::~DeviceOffHoursController() {
  for (auto& observer : observers_)
    observer.OffHoursControllerShutDown();
  chromeos::system::TimezoneSettings::GetInstance()->RemoveObserver(this);
  if (chromeos::DBusThreadManager::IsInitialized() &&
      chromeos::DBusThreadManager::Get()->GetSessionManagerClient()) {
    chromeos::DBusThreadManager::Get()
        ->GetSessionManagerClient()
        ->RemoveObserver(this);
  }
  StopOffHoursTimer();
  Shutdown();
  LOG(ERROR) << "Daria ~DeviceOffHoursController";
}

// Timer, delay in milliseconds
void DeviceOffHoursController::StartOffHoursTimer(int delay) {
  timer_.Start(FROM_HERE, base::TimeDelta::FromMilliseconds(delay),
               base::Bind(&DeviceOffHoursController::NotifyOffHoursModeChanged,
                          base::Unretained(this)));
  LOG(ERROR) << "Daria Timer started with delay(minutes) = "
             << delay / 1000 / 60;
}

void DeviceOffHoursController::StopOffHoursTimer() {
  timer_.Stop();
}

void DeviceOffHoursController::TimezoneChanged(const icu::TimeZone& timezone) {
  if (!IsInitialized()) {
    return;
  }
  LOG(ERROR) << "Daria TimeZone changed";
  NotifyOffHoursModeChanged();
}

void DeviceOffHoursController::ScreenIsUnlocked() {
  if (!IsInitialized()) {
    return;
  }
  LOG(ERROR) << "Daria screen is unlocked";
  NotifyOffHoursModeChanged();
}

void DeviceOffHoursController::NotifyOffHoursModeChanged() const {
  LOG(ERROR) << "Daria NotifyOffHoursModeChanged";
  for (auto& observer : observers_)
    observer.OffHoursModeChanged();
}

// static
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
  if (!IsInitialized()) {
    Initialize();
  }
  const em::DeviceOffHoursProto& container(input_policies->device_off_hours());
  std::vector<std::unique_ptr<Interval>> intervals = get_intervals(container);
  LOG(ERROR) << "Daria: INTERVAL COUNT " << intervals.size();
  bool is_off_hours =
      is_off_hours_mode(g_device_off_hours_controller, intervals);
  if (!is_off_hours || container.ignored_policy_size() == 0) {
    return nullptr;
  }
  LOG(ERROR) << "Daria OFF HOURS MODE ON";
  std::set<std::string> ignored_policies;
  for (const auto& p : container.ignored_policy()) {
    ignored_policies.insert(p);
  }
  em::ChromeDeviceSettingsProto policies =
      clear_policies(input_policies, ignored_policies);
  return base::MakeUnique<em::ChromeDeviceSettingsProto>(policies);
}
}  // namespace policy
