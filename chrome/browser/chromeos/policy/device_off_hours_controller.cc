// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/chromeos/policy/device_off_hours_controller.h"

#include <regex>
#include <thread>
#include <utility>

#include "base/strings/utf_string_conversions.h"
#include "chrome/browser/chromeos/policy/device_policy_remover.h"
#include "chrome/browser/chromeos/system/timezone_util.h"
#include "chrome/browser/lifetime/application_lifetime.h"
#include "chromeos/dbus/dbus_thread_manager.h"
#include "chromeos/login/login_state.h"
#include "chromeos/settings/timezone_settings.h"
#include "components/policy/core/common/cloud/cloud_policy_store.h"
#include "components/user_manager/user_manager.h"
#include "unicode/gregocal.h"

namespace em = enterprise_management;

namespace policy {
namespace {

const int kMillisecondsInHour = base::TimeDelta::FromHours(1).InMilliseconds();
const int kMillisecondsInDay = base::TimeDelta::FromDays(1).InMilliseconds();
const int kMillisecondsInWeek = base::TimeDelta::FromDays(7).InMilliseconds();

// WeeklyTime class contains weekday (1..7) anf time in milliseconds.
class WeeklyTime {
 public:
  int weekday;
  // Time of day in milliseconds from the beginning of the day.
  int milliseconds;

  WeeklyTime() {}

  WeeklyTime(int w, int ms) : weekday(w), milliseconds(ms) {}

  // Constructor converts input time in timezone |timezone|
  // to UTC considering daylight time.
  WeeklyTime(int w, int ms, const std::string& timezone) {
    if (timezone == "UTC") {
      weekday = w;
      milliseconds = ms;
      return;
    }
    icu::TimeZone* zone =
        icu::TimeZone::createTimeZone(icu::UnicodeString::fromUTF8(timezone));
    // Time delta is in milliseconds.
    int deltaMS = -(zone->getRawOffset());
    UErrorCode status;
    icu::GregorianCalendar* gc = new icu::GregorianCalendar(zone, status);
    UBool day_light = gc->inDaylightTime(status);
    delete zone;
    delete gc;
    LOG(ERROR) << "Daria: daylight: " << (bool)day_light
               << "; status=" << u_errorName(status)
               << ", timezone=" << timezone;
    if (day_light) {
      LOG(ERROR) << "Daria: Day Light Time!! timezone = " << timezone;
      deltaMS -= kMillisecondsInHour;
    }
    deltaMS = kMillisecondsInWeek + deltaMS;
    milliseconds = (ms + deltaMS) % kMillisecondsInDay;
    int deltaW = (ms + deltaMS) / kMillisecondsInDay;
    weekday = (w + deltaW - 1) % 7 + 1;
  }

  std::unique_ptr<base::DictionaryValue> ToValue() {
    auto weekly_time = base::MakeUnique<base::DictionaryValue>();
    weekly_time->SetInteger("weekday", weekday);
    weekly_time->SetInteger("time", milliseconds);
    return weekly_time;
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

  // Time delta between two weekly times in milliseconds.
  int operator-(const WeeklyTime& wt) const {
    int delta_m = 0;
    if (*this > wt) {
      // Days delta.
      delta_m += (weekday - wt.weekday - 1) * kMillisecondsInDay;
      // Milliseconds delta.
      delta_m += milliseconds + (kMillisecondsInDay - wt.milliseconds);
      return delta_m;
    }
    return kMillisecondsInWeek - (wt - *this);
  }
};

// Time interval structure, interval = [start, end].
class Interval {
 public:
  WeeklyTime start;
  WeeklyTime end;

  Interval() {}

  Interval(WeeklyTime st, WeeklyTime en) : start(st), end(en) {}

  std::string ToString() const {
    return "[" + start.ToString() + "; " + end.ToString() + "]";
  }

  std::unique_ptr<base::DictionaryValue> ToValue() {
    auto interval = base::MakeUnique<base::DictionaryValue>();
    interval->SetDictionary("start", start.ToValue());
    interval->SetDictionary("end", end.ToValue());
    return interval;
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

// Get WeeklyTime structure from WeeklyTimeProto and put it to |weekly_time|.
// Return false if |weekly_time| isn't correct.
bool GetWeeklyTime(WeeklyTime* weekly_time,
                   const em::WeeklyTimeProto& container,
                   const std::string& timezone) {
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
  *weekly_time = WeeklyTime(container.weekday(), time_of_day, timezone);
  LOG(ERROR) << "Daria UTC time: " << weekly_time->ToString();
  return true;
}

// Get and return list of time intervals from DeviceOffHoursProto structure.
std::vector<std::unique_ptr<Interval>> GetIntervals(
    const em::DeviceOffHoursProto& container,
    const std::string& timezone = "UTC") {
  std::vector<std::unique_ptr<Interval>> intervals;
  if (container.interval_size() == 0)
    return intervals;
  for (const auto& entry : container.interval()) {
    if (!entry.has_start() || !entry.has_end())
      continue;
    Interval interval;
    if (GetWeeklyTime(&interval.start, entry.start(), timezone) &&
        GetWeeklyTime(&interval.end, entry.end(), timezone)) {
      intervals.push_back(base::MakeUnique<Interval>(interval));
    }
  }
  return intervals;
}

// Get and return list of ignored policies from DeviceOffHoursProto structure.
std::vector<std::string> GetIgnoredPolicies(
    const em::DeviceOffHoursProto& container) {
  std::vector<std::string> ignored_policies;
  if (container.ignored_policy_size() == 0)
    return ignored_policies;
  for (const auto& entry : container.ignored_policy()) {
    ignored_policies.push_back(entry);
  }
  return ignored_policies;
}

// Go through interval and check if "OffHours" mode is in current time.
bool CheckOffHoursMode(
    const std::vector<std::unique_ptr<Interval>>& intervals) {
  const time_t theTime = base::Time::Now().ToTimeT();
  struct tm* aTime = gmtime(&theTime);
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

// Timer
// |delay| value in milliseconds.
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

void DeviceOffHoursController::NotifyOffHoursModeChanged() const {
  LOG(ERROR) << "Daria NotifyOffHoursModeChanged";
  for (auto& observer : observers_)
    observer.OffHoursModeChanged();
}

void DeviceOffHoursController::ScreenIsUnlocked() {
  if (!IsInitialized()) {
    return;
  }
  LOG(ERROR) << "Daria screen is unlocked";
  NotifyOffHoursModeChanged();
}

// Call chrome::AttemptUserExit() if guest mode or current user
// aren't enabled more.
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
  std::vector<std::string> ignored_policies = GetIgnoredPolicies(container);
  auto ignored_policies_value = base::MakeUnique<base::ListValue>();
  for (const auto& policy : ignored_policies) {
    ignored_policies_value->AppendString(policy);
  }
  off_hours->SetList("ignored_policies", std::move(ignored_policies_value));
  return off_hours;
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
  const em::DeviceOffHoursProto& container(input_policies->device_off_hours());
  if (!container.has_timezone())
    return nullptr;
  if (!IsInitialized()) {
    Initialize();
  }
  device_policies = input_policies;
  LOG(ERROR) << "Daria OFF HOURS POLICIES " << Get()->IsOffHoursMode();
  std::vector<std::unique_ptr<Interval>> intervals =
      GetIntervals(container, container.timezone());
  LOG(ERROR) << "Daria: INTERVAL COUNT " << intervals.size();
  std::vector<std::string> ignored_policies = GetIgnoredPolicies(container);
  if (intervals.size() == 0 || ignored_policies.size() == 0) {
    return nullptr;
  }
  bool is_off_hours = CheckOffHoursMode(intervals);
  if (!is_off_hours) {
    return nullptr;
  }
  LOG(ERROR) << "Daria OFF HOURS MODE ON";
  em::ChromeDeviceSettingsProto policies;
  policies.CopyFrom(*input_policies);
  RemovePolicies(&policies, &ignored_policies);
  return base::MakeUnique<em::ChromeDeviceSettingsProto>(policies);
}

}  // namespace policy
