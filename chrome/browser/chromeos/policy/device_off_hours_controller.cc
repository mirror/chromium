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
// (int)base::Time::kMicrosecondsPerHour;
const int kMillisecondsInDay = base::TimeDelta::FromDays(1).InMilliseconds();
// (int)base::Time::kMicrosecondsPerDay;
const int kMillisecondsInWeek = base::TimeDelta::FromDays(7).InMilliseconds();
// (int)base::Time::kMicrosecondsPerWeek;

// WeeklyTime class contains weekday (1..7) anf time in milliseconds.
class WeeklyTime {
 public:
  int weekday;
  // Time of day in milliseconds from beginning of the day.
  int milliseconds;
  WeeklyTime() {}
  WeeklyTime(int w, int ms) : weekday(w), milliseconds(ms) {}
  WeeklyTime(int w, int ms, const std::string& timezone) {
    icu::TimeZone* zone =
        icu::TimeZone::createTimeZone(icu::UnicodeString::fromUTF8(timezone));
    // delta time in milliseconds
    int deltaMS = -(zone->getRawOffset());
    UErrorCode status;
    icu::GregorianCalendar* gc = new icu::GregorianCalendar(zone, status);
    UBool day_light = gc->inDaylightTime(status);
    LOG(ERROR) << "Daria: daylight: " << (bool)day_light
               << "; status=" << (int)status;
    if (day_light) {
      LOG(ERROR) << "Daria: Day Light Time!! timezone = " << timezone;
      deltaMS -= kMillisecondsInHour;
    }
    deltaMS = kMillisecondsInWeek + deltaMS;
    milliseconds = (ms + deltaMS) % kMillisecondsInDay;
    int deltaW = (ms + deltaMS) / kMillisecondsInDay;
    weekday = (w + deltaW - 1) % 7 + 1;
  }
  std::string ToString() const {
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
  std::string ToString() const {
    return "[" + start.ToString() + "; " + end.ToString() + "]";
  }
};

// Check if WeeklyTime |w| in interval |interval|
bool InInterval(const Interval* interval, const WeeklyTime* w) {
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
bool GetWeeklyTime(WeeklyTime* weekly_time,
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
  LOG(ERROR) << "Daria UTC time: " << weekly_time->ToString();
  return true;
}

// Get and return list of time intervals from DeviceOffHoursProto structure
std::vector<std::unique_ptr<Interval>> GetIntervals(
    const em::DeviceOffHoursProto& container) {
  std::string timezone = "GMT";
  if (container.has_timezone()) {
    timezone = container.timezone();
  }
  LOG(ERROR) << "Daria get intervals from timezone = " << timezone;
  std::vector<std::unique_ptr<Interval>> intervals;
  if (container.interval_size() == 0) {
    return intervals;
  }
  for (const auto& entry : container.interval()) {
    Interval interval;
    if (entry.has_start() && entry.has_end()) {
      if (GetWeeklyTime(&interval.start, entry.start(), timezone) &&
          GetWeeklyTime(&interval.end, entry.end(), timezone)) {
        intervals.push_back(base::MakeUnique<Interval>(interval));
      }
    }
  }
  return intervals;
}

// Go through interval and check off_hours mode
bool CheckOffHoursMode(
    const std::vector<std::unique_ptr<Interval>>& intervals) {
  const time_t theTime = base::Time::Now().ToTimeT();
  struct tm* aTime = gmtime(&theTime);
  // TODO(yakovleva) find better convertation
  WeeklyTime cur_t(aTime->tm_wday,
                   (aTime->tm_hour * 60 + aTime->tm_min) * 60 * 1000);
  LOG(ERROR) << "Daria CURRENT TIME UTC:" << cur_t.ToString();
  int before_off_hours = kMillisecondsInWeek;
  for (size_t i = 0; i < intervals.size(); i++) {
    LOG(ERROR) << "Daria interval " << intervals[i]->ToString();
    if (InInterval(intervals[i].get(), &cur_t)) {
      DeviceOffHoursController::Get()->SetOffHoursMode();
      DeviceOffHoursController::Get()->StartOffHoursTimer(intervals[i]->end -
                                                          cur_t + 1);
      return true;
    }
    before_off_hours = std::min(before_off_hours, intervals[i]->start - cur_t);
  }
  if (before_off_hours < kMillisecondsInWeek) {
    DeviceOffHoursController::Get()->StartOffHoursTimer(before_off_hours);
  }
  DeviceOffHoursController::Get()->UnsetOffHoursMode();
  return false;
}

em::ChromeDeviceSettingsProto* device_policies = NULL;

}  // namespace

static DeviceOffHoursController* g_device_off_hours_controller = NULL;

DeviceOffHoursController::DeviceOffHoursController() {
  off_hours_mode_ = false;
  if (chromeos::DBusThreadManager::IsInitialized() &&
      chromeos::DBusThreadManager::Get()->GetSessionManagerClient()) {
    chromeos::DBusThreadManager::Get()->GetSessionManagerClient()->AddObserver(
        this);
  }
}

DeviceOffHoursController::~DeviceOffHoursController() {
  for (auto& observer : observers_)
    observer.OffHoursControllerShutDown();
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
  LOG(ERROR) << "  Daria controller id = " << g_device_off_hours_controller;
  LOG(ERROR) << "  Daria off hours mode = "
             << g_device_off_hours_controller->off_hours_mode_;
}

// static
bool DeviceOffHoursController::IsInitialized() {
  return g_device_off_hours_controller != NULL;
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

// Timer, delay in milliseconds
void DeviceOffHoursController::StartOffHoursTimer(int delay) {
  if (timer_.IsRunning()) {
    int timer_delay = timer_.GetCurrentDelay().InMilliseconds();
    if (timer_delay < delay) {
      return;
    }
    timer_.Stop();
  }
  timer_.Start(FROM_HERE, base::TimeDelta::FromMilliseconds(delay),
               base::Bind(&DeviceOffHoursController::NotifyOffHoursModeChanged,
                          base::Unretained(this)));
  LOG(ERROR) << "Daria Timer started with delay(minutes) = "
             << delay / 1000 / 60;
}

void DeviceOffHoursController::StopOffHoursTimer() {
  timer_.Stop();
}

void DeviceOffHoursController::ScreenIsUnlocked() {
  if (!IsInitialized()) {
    return;
  }
  LOG(ERROR) << "Daria screen is unlocked";
  NotifyOffHoursModeChanged();
}

// Call chrome::AttemptUserExit() if guest mode ar current user aren't enabled
// more
void DeviceOffHoursController::LogoutUserIfNeed() {
  CHECK(device_policies);
  bool is_log_in = chromeos::LoginState::Get()->IsUserLoggedIn();
  bool is_guest = chromeos::LoginState::Get()->IsGuestSessionUser();
  if (is_guest) {
    if (device_policies->has_guest_mode_enabled()) {
      const em::GuestModeEnabledProto& container(
          device_policies->guest_mode_enabled());
      if (container.has_guest_mode_enabled() &&
          !container.guest_mode_enabled()) {
        LOG(ERROR) << "Daria: guest mode turn off, attempt user exit";
        chrome::AttemptUserExit();
        return;
      }
    }
    return;
  }
  if (!is_log_in) {
    return;
  }
  const std::string current_user = user_manager::UserManager::Get()
                                       ->GetActiveUser()
                                       ->GetAccountId()
                                       .GetUserEmail();
  if (device_policies->has_user_whitelist()) {
    const em::UserWhitelistProto& container(device_policies->user_whitelist());
    bool user_is_allowed = false;
    for (const std::string& entry : container.user_whitelist()) {
      if (std::regex_match(current_user, std::regex(entry))) {
        user_is_allowed = true;
      }
    }
    if (!user_is_allowed) {
      LOG(ERROR) << "Daria: user isn't allowed, attempt user exit";
      chrome::AttemptUserExit();
    }
  }
}

void DeviceOffHoursController::NotifyOffHoursModeChanged() const {
  LOG(ERROR) << "Daria NotifyOffHoursModeChanged";
  for (auto& observer : observers_)
    observer.OffHoursModeChanged();
}

bool DeviceOffHoursController::IsOffHoursMode() {
  return off_hours_mode_;
}

void DeviceOffHoursController::SetOffHoursMode() {
  off_hours_mode_ = true;
}

void DeviceOffHoursController::UnsetOffHoursMode() {
  if (!off_hours_mode_) {
    return;
  }
  off_hours_mode_ = false;
  LogoutUserIfNeed();
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
  if (!IsInitialized()) {
    Initialize();
  }
  device_policies = input_policies;
  LOG(ERROR) << "Daria OFF HOURS POLICIES " << Get()->IsOffHoursMode();
  const em::DeviceOffHoursProto& container(input_policies->device_off_hours());
  std::vector<std::unique_ptr<Interval>> intervals = GetIntervals(container);
  LOG(ERROR) << "Daria: INTERVAL COUNT " << intervals.size();
  bool is_off_hours = CheckOffHoursMode(intervals);
  if (!is_off_hours || container.ignored_policy_size() == 0) {
    return nullptr;
  }
  LOG(ERROR) << "Daria OFF HOURS MODE ON";
  std::set<std::string> ignored_policies;
  for (const auto& p : container.ignored_policy()) {
    ignored_policies.insert(p);
  }
  em::ChromeDeviceSettingsProto policies =
      ClearPolicies(input_policies, ignored_policies);
  return base::MakeUnique<em::ChromeDeviceSettingsProto>(policies);
}
}  // namespace policy
