// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/chromeos/policy/device_off_hours_controller.h"

#include <regex>
#include <string>
#include <thread>
#include <utility>
#include <vector>

#include "base/logging.h"
#include "base/memory/ptr_util.h"
#include "base/strings/utf_string_conversions.h"
#include "base/time/time.h"
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

// WeeklyTime class contains weekday and time in milliseconds.
class WeeklyTime {
 public:
  WeeklyTime(int weekday, int milliseconds)
      : weekday_(weekday), milliseconds_(milliseconds) {}

  // Constructor converts input time in timezone |timezone|
  // to UTC considering daylight time.
  WeeklyTime(int weekday, int milliseconds, const std::string& timezone) {
    if (timezone == "UTC") {
      weekday_ = weekday;
      milliseconds_ = milliseconds;
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
    milliseconds_ = (milliseconds + deltaMS) % kMillisecondsInDay;
    int deltaW = (milliseconds + deltaMS) / kMillisecondsInDay;
    weekday_ = (weekday + deltaW - 1) % 7 + 1;
  }

  int getWeekday() const { return weekday_; }

  int getMilliseconds() const { return milliseconds_; }

  std::unique_ptr<base::DictionaryValue> ToValue() const {
    auto weekly_time = base::MakeUnique<base::DictionaryValue>();
    weekly_time->SetInteger("weekday", weekday_);
    weekly_time->SetInteger("time", milliseconds_);
    return weekly_time;
  }

  std::string ToString() const {
    return "weekday: " + std::to_string(weekday_) + ", " +
           std::to_string(milliseconds_ / kMillisecondsInHour) + ":" +
           std::to_string((milliseconds_ % kMillisecondsInHour) / 1000 / 60);
  }

  bool operator==(const WeeklyTime& wt) const {
    return (weekday_ == wt.getWeekday()) &&
           (milliseconds_ == wt.getMilliseconds());
  }

  bool operator<(const WeeklyTime& wt) const {
    return (weekday_ < wt.getWeekday()) ||
           (weekday_ == wt.getWeekday() &&
            milliseconds_ < wt.getMilliseconds());
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
      delta_m += (weekday_ - wt.getWeekday() - 1) * kMillisecondsInDay;
      // Milliseconds delta.
      delta_m += milliseconds_ + (kMillisecondsInDay - wt.getMilliseconds());
      return delta_m;
    }
    return kMillisecondsInWeek - (wt - *this);
  }

 private:
  // Number of weekday (1 = Monday, 2 = Tuesday, etc.)
  int weekday_;
  // Time of day in milliseconds from the beginning of the day.
  int milliseconds_;
};

// Time interval structure, interval = [start, end].
struct Interval {
  std::unique_ptr<WeeklyTime> start;
  std::unique_ptr<WeeklyTime> end;

  Interval(Interval& interval)
      : start(std::move(interval.start)), end(std::move(interval.end)) {}

  Interval(const WeeklyTime& start, const WeeklyTime& end)
      : start(base::MakeUnique<WeeklyTime>(start)),
        end(base::MakeUnique<WeeklyTime>(end)) {}

  std::string ToString() const {
    return "[" + start->ToString() + "; " + end->ToString() + "]";
  }

  std::unique_ptr<base::DictionaryValue> ToValue() const {
    auto interval = base::MakeUnique<base::DictionaryValue>();
    interval->SetDictionary("start", start->ToValue());
    interval->SetDictionary("end", end->ToValue());
    return interval;
  }
};

// Check if WeeklyTime |w| in interval |interval|
bool InInterval(const Interval& interval, const WeeklyTime& w) {
  if (*interval.start == *interval.end) {
    if (*interval.start == w) {
      return true;
    }
  } else if (*interval.start < *interval.end) {
    if (w >= *interval.start && w <= *interval.end) {
      return true;
    }
  } else {
    if (w >= *interval.start || w <= *interval.end) {
      return true;
    }
  }
  return false;
}

// Get WeeklyTime structure from WeeklyTimeProto and put it to |weekly_time|
// Return false if |weekly_time| won't correct
std::unique_ptr<WeeklyTime> GetWeeklyTime(
    const em::WeeklyTimeProto& container) {
  if (!container.has_weekday() ||
      container.weekday() == em::WeeklyTimeProto::DAY_OF_WEEK_UNSPECIFIED) {
    LOG(ERROR) << "Day of week in interval is absent or unspecified.";
    return nullptr;
  }
  if (!container.has_time()) {
    LOG(ERROR) << "Time in interval is absent.";
    return nullptr;
  }
  int time_of_day = container.time();
  if (!(time_of_day >= 0 && time_of_day < kMillisecondsInDay)) {
    LOG(ERROR) << "Invalid time value: " << time_of_day
               << ", the value should be in [0; " << kMillisecondsInDay << ").";
    return nullptr;
  }
  return base::MakeUnique<WeeklyTime>(container.weekday(), time_of_day);
}

// Get and return list of time intervals from DeviceOffHoursProto structure.
std::vector<std::unique_ptr<Interval>> GetIntervals(
    const em::DeviceOffHoursProto& container,
    const std::string& timezone = "UTC") {
  std::vector<std::unique_ptr<Interval>> intervals;
  for (const auto& entry : container.interval()) {
    if (!entry.has_start() || !entry.has_end()) {
      LOG(WARNING) << "Miss interval without start or/and end.";
      continue;
    }
    auto start = GetWeeklyTime(entry.start());
    auto end = GetWeeklyTime(entry.end());
    if (start && end) {
      intervals.push_back(base::MakeUnique<Interval>(*start, *end));
    }
  }
  return intervals;
}

// Get and return list of ignored policies from DeviceOffHoursProto structure.
std::vector<std::string> GetIgnoredPolicies(
    const em::DeviceOffHoursProto& container) {
  std::vector<std::string> ignored_policies;
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
    if (InInterval(*intervals[i], cur_t)) {
      DeviceOffHoursController::Get()->SetOffHoursMode();
      DeviceOffHoursController::Get()->StartOffHoursTimer(*intervals[i]->end -
                                                          cur_t + 1);
      return true;
    }
    before_off_hours = std::min(before_off_hours, *intervals[i]->start - cur_t);
  }
  if (before_off_hours < kMillisecondsInWeek) {
    DeviceOffHoursController::Get()->StartOffHoursTimer(before_off_hours);
  }
  DeviceOffHoursController::Get()->UnsetOffHoursMode();
  return false;
}

em::ChromeDeviceSettingsProto* device_policies = NULL;

}  // namespace

namespace off_hours {

std::unique_ptr<base::DictionaryValue> ConvertToValue(
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
}  // namespace off_hours

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
