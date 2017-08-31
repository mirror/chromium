// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/chromeos/policy/device_off_hours_controller.h"

#include <string>
#include <utility>
#include <vector>

#include "base/logging.h"
#include "base/memory/ptr_util.h"
#include "base/strings/utf_string_conversions.h"
#include "base/time/time.h"
#include "chrome/browser/chromeos/policy/device_policy_remover.h"
#include "chrome/browser/chromeos/settings/cros_settings.h"
#include "chrome/browser/lifetime/application_lifetime.h"
#include "chromeos/dbus/dbus_thread_manager.h"
#include "components/user_manager/user_manager.h"
#include "unicode/gregocal.h"

namespace em = enterprise_management;

namespace policy {
namespace {

const int kMillisecondsInMinute =
    base::TimeDelta::FromMinutes(1).InMilliseconds();
const int kMillisecondsInHour = base::TimeDelta::FromHours(1).InMilliseconds();
const int kMillisecondsInDay = base::TimeDelta::FromDays(1).InMilliseconds();
const int kMillisecondsInWeek = base::TimeDelta::FromDays(7).InMilliseconds();

// WeeklyTime class contains day_of_week and time in milliseconds.
class WeeklyTime {
 public:
  WeeklyTime(int day_of_week, int milliseconds)
      : day_of_week_(day_of_week), milliseconds_(milliseconds) {}

  // Constructor converts input time in |timezone|
  // to UTC considering daylight time.
  WeeklyTime(int day_of_week, int milliseconds, const std::string& timezone) {
    if (timezone == "UTC") {
      day_of_week_ = day_of_week;
      milliseconds_ = milliseconds;
      return;
    }
    icu::TimeZone* zone =
        icu::TimeZone::createTimeZone(icu::UnicodeString::fromUTF8(timezone));
    // Time delta is in milliseconds.
    int deltaMS = -(zone->getRawOffset());
    int delta_DST = zone->getDSTSavings();
    UErrorCode status = U_ZERO_ERROR;
    icu::GregorianCalendar* gc = new icu::GregorianCalendar(zone, status);
    status = U_ZERO_ERROR;
    UBool day_light = gc->inDaylightTime(status);
    if (U_FAILURE(status)) {
      LOG(ERROR) << "Daylight time error = " << u_errorName(status);
    }
    if (day_light) {
      deltaMS -= delta_DST;
    }
    deltaMS = kMillisecondsInWeek + deltaMS;
    milliseconds_ = (milliseconds + deltaMS) % kMillisecondsInDay;
    int deltaW = (milliseconds + deltaMS) / kMillisecondsInDay;
    day_of_week_ = (day_of_week + deltaW - 1) % 7 + 1;
  }

  int getDayOfWeek() const { return day_of_week_; }

  int getMilliseconds() const { return milliseconds_; }

  std::unique_ptr<base::DictionaryValue> ToValue() const {
    auto weekly_time = base::MakeUnique<base::DictionaryValue>();
    weekly_time->SetInteger("day_of_week", day_of_week_);
    weekly_time->SetInteger("time", milliseconds_);
    return weekly_time;
  }

  std::string ToString() const {
    return "day_of_week: " + std::to_string(day_of_week_) + ", " +
           std::to_string(milliseconds_ / kMillisecondsInHour) + ":" +
           std::to_string((milliseconds_ % kMillisecondsInHour) /
                          kMillisecondsInMinute);
  }

  bool operator==(const WeeklyTime& wt) const {
    return (day_of_week_ == wt.getDayOfWeek()) &&
           (milliseconds_ == wt.getMilliseconds());
  }

  bool operator<(const WeeklyTime& wt) const {
    return (day_of_week_ < wt.getDayOfWeek()) ||
           (day_of_week_ == wt.getDayOfWeek() &&
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
  // This value is always positive or zero because
  // If *this < |wt| then it means that
  // *this is next week time.
  int operator-(const WeeklyTime& wt) const {
    int delta_m = 0;
    if (*this > wt) {
      // Days delta.
      delta_m += (day_of_week_ - wt.getDayOfWeek() - 1) * kMillisecondsInDay;
      // Milliseconds delta.
      delta_m += milliseconds_ + (kMillisecondsInDay - wt.getMilliseconds());
      return delta_m;
    }
    return kMillisecondsInWeek - (wt - *this);
  }

 private:
  // Number of weekday (1 = Monday, 2 = Tuesday, etc.)
  int day_of_week_;

  // Time of day in milliseconds from the beginning of the day.
  int milliseconds_;
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

  std::string ToString() const {
    return "[" + start.ToString() + "; " + end.ToString() + "]";
  }

  WeeklyTime start;
  WeeklyTime end;
};

// Check if |w| is in [interval.start, interval.end).
// If interval.start > interval.end then
//   interval.end is next week time. (i.e. [Friday 17:00, Monday 9:00) )
bool InInterval(const Interval& interval, const WeeklyTime& w) {
  if (interval.start == interval.end) {
    return false;
  } else if (interval.start < interval.end) {
    if (w >= interval.start && w < interval.end) {
      return true;
    }
  } else {
    if (w >= interval.start || w < interval.end) {
      return true;
    }
  }
  return false;
}

// Get and return WeeklyTime structure from WeeklyTimeProto
// Return nullptr if WeeklyTime structure isn't correct.
std::unique_ptr<WeeklyTime> GetWeeklyTime(const em::WeeklyTimeProto& container,
                                          const std::string& timezone) {
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
  if (!(time_of_day >= 0 && time_of_day < kMillisecondsInDay)) {
    LOG(ERROR) << "Invalid time value: " << time_of_day
               << ", the value should be in [0; " << kMillisecondsInDay << ").";
    return nullptr;
  }
  return base::MakeUnique<WeeklyTime>(container.day_of_week(), time_of_day,
                                      timezone);
}

// Get and return list of time intervals from DeviceOffHoursProto structure.
std::vector<Interval> GetIntervals(const em::DeviceOffHoursProto& container,
                                   const std::string& timezone = "UTC") {
  std::vector<Interval> intervals;
  for (const auto& entry : container.intervals()) {
    if (!entry.has_start() || !entry.has_end()) {
      LOG(WARNING) << "Skipping interval without start or/and end.";
      continue;
    }
    auto start = GetWeeklyTime(entry.start(), timezone);
    auto end = GetWeeklyTime(entry.end(), timezone);
    if (start && end) {
      intervals.push_back(Interval(*start, *end));
    }
  }
  return intervals;
}

// Get and return list of ignored policies from DeviceOffHoursProto structure.
std::vector<std::string> GetIgnoredPolicies(
    const em::DeviceOffHoursProto& container) {
  std::vector<std::string> ignored_policies;
  return std::vector<std::string>(container.ignored_policies().begin(),
                                  container.ignored_policies().end());
}

// Check if current user is allowed based on settings in |device_policies|.
// Checked policies: guest_mode_enabled, allow_new_users, user_whitelist.
bool IsCurrentUserAllowed(
    const em::ChromeDeviceSettingsProto& device_policies) {
  bool is_guest = user_manager::UserManager::Get()->IsLoggedInAsGuest();
  if (is_guest) {
    if (device_policies.has_guest_mode_enabled()) {
      const em::GuestModeEnabledProto& container(
          device_policies.guest_mode_enabled());
      if (container.has_guest_mode_enabled() &&
          !container.guest_mode_enabled()) {
        return false;
      }
    }
    return true;
  }
  const std::string current_user = user_manager::UserManager::Get()
                                       ->GetActiveUser()
                                       ->GetAccountId()
                                       .GetUserEmail();
  bool allow_new_users = false;
  if (device_policies.has_allow_new_users()) {
    const em::AllowNewUsersProto& container(device_policies.allow_new_users());
    if (container.allow_new_users())
      return true;
  } else {
    allow_new_users = true;
  }
  if (device_policies.has_user_whitelist()) {
    const em::UserWhitelistProto& container(device_policies.user_whitelist());
    base::ListValue* whitelist = new base::ListValue();
    for (const auto& entry : container.user_whitelist())
      whitelist->AppendString(entry);
    if (allow_new_users && whitelist->empty()) {
      return true;
    }
    bool user_in_whitelist = chromeos::CrosSettings::Get()->FindEmailInList(
        whitelist, current_user, NULL);
    if (!user_in_whitelist)
      return false;
  }
  return true;
}

const em::ChromeDeviceSettingsProto* device_policies = NULL;

}  // namespace

namespace off_hours {

std::unique_ptr<base::DictionaryValue> ConvertPolicyProtoToValue(
    const em::DeviceOffHoursProto& container) {
  if (!container.has_timezone())
    return nullptr;
  auto off_hours = base::MakeUnique<base::DictionaryValue>();
  off_hours->SetString("timezone", container.timezone());
  std::vector<Interval> intervals = GetIntervals(container);
  auto intervals_value = base::MakeUnique<base::ListValue>();
  for (const auto& interval : intervals) {
    intervals_value->Append(interval.ToValue());
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

void DeviceOffHoursController::Observer::OffHoursModeMaybeChanged() {}

void DeviceOffHoursController::AddObserver(Observer* observer) {
  observers_.AddObserver(observer);
}

void DeviceOffHoursController::RemoveObserver(Observer* observer) {
  observers_.RemoveObserver(observer);
}

DeviceOffHoursController::DeviceOffHoursController() {
  off_hours_mode_ = false;
  chromeos::DBusThreadManager::Get()->GetSessionManagerClient()->AddObserver(
      this);
}

DeviceOffHoursController::~DeviceOffHoursController() {
  for (auto& observer : observers_)
    observer.OffHoursControllerShutDown();
  chromeos::DBusThreadManager::Get()->GetSessionManagerClient()->RemoveObserver(
      this);
  StopOffHoursTimer();
  Shutdown();
}

// static
void DeviceOffHoursController::Initialize() {
  CHECK(!g_device_off_hours_controller);
  g_device_off_hours_controller = new DeviceOffHoursController();
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
  if (off_hours_mode_)
    return;
  VLOG(1) << "OffHours mode is on.";
  off_hours_mode_ = true;
}

void DeviceOffHoursController::UnsetOffHoursMode() {
  if (!off_hours_mode_) {
    return;
  }
  VLOG(1) << "OffHours mode is off.";
  off_hours_mode_ = false;
  CHECK(device_policies);
  LogoutUserIfNeed(*device_policies);
}

// Timer
// |delay| value in milliseconds.
void DeviceOffHoursController::StartOffHoursTimer(int delay) {
  if (delay <= 0) {
    LOG(ERROR) << "Attempt to set timer with delay = " << delay;
    return;
  }
  if (timer_.IsRunning()) {
    int timer_delay = timer_.GetCurrentDelay().InMilliseconds();
    if (timer_delay < delay) {
      return;
    }
    timer_.Stop();
  }
  VLOG(1) << "OffHours mode timer starts for " << delay << " milliseconds.";
  timer_.Start(FROM_HERE, base::TimeDelta::FromMilliseconds(delay),
               base::Bind(&DeviceOffHoursController::NotifyOffHoursModeChanged,
                          base::Unretained(this)));
}

void DeviceOffHoursController::StopOffHoursTimer() {
  timer_.Stop();
}

void DeviceOffHoursController::NotifyOffHoursModeChanged() const {
  if (!IsInitialized()) {
    return;
  }
  VLOG(1) << "OffHours mode is probably changed.";
  for (auto& observer : observers_)
    observer.OffHoursModeMaybeChanged();
}

void DeviceOffHoursController::ScreenIsUnlocked() {
  NotifyOffHoursModeChanged();
}

// Call chrome::AttemptUserExit()
// if guest mode or current user aren't enabled more.
void DeviceOffHoursController::LogoutUserIfNeed(
    const em::ChromeDeviceSettingsProto& policies) {
  bool is_log_in = user_manager::UserManager::Get()->IsUserLoggedIn();
  if (!is_log_in) {
    return;
  }
  if (!IsCurrentUserAllowed(policies)) {
    VLOG(1) << "User isn't allow. Attempt user exit.";
    chrome::AttemptUserExit();
  }
}

// static
// Check if "OffHours" mode is in current time.
// Update current status in |off_hours_mode_|.
// Set timer to next update "OffHours" mode.
void DeviceOffHoursController::UpdateOffHoursMode(
    const em::ChromeDeviceSettingsProto& input_policies) {
  DeviceOffHoursController* off_hours_controller =
      DeviceOffHoursController::Get();
  if (!input_policies.has_device_off_hours()) {
    off_hours_controller->UnsetOffHoursMode();
    return;
  }
  const em::DeviceOffHoursProto& container(input_policies.device_off_hours());
  if (!container.has_timezone()) {
    off_hours_controller->UnsetOffHoursMode();
    return;
  }
  std::vector<Interval> intervals =
      GetIntervals(container, container.timezone());
  device_policies = &input_policies;
  const time_t theTime = base::Time::Now().ToTimeT();
  struct tm* aTime = gmtime(&theTime);
  WeeklyTime cur_t(aTime->tm_wday, (aTime->tm_hour * 60 + aTime->tm_min) *
                                           kMillisecondsInMinute +
                                       aTime->tm_sec * 1000);
  int before_off_hours = kMillisecondsInWeek;
  for (size_t i = 0; i < intervals.size(); i++) {
    if (InInterval(intervals[i], cur_t)) {
      off_hours_controller->SetOffHoursMode();
      off_hours_controller->StartOffHoursTimer(intervals[i].end - cur_t);
      return;
    }
    before_off_hours = std::min(before_off_hours, intervals[i].start - cur_t);
  }
  if (before_off_hours < kMillisecondsInWeek) {
    off_hours_controller->StartOffHoursTimer(before_off_hours);
  }
  off_hours_controller->UnsetOffHoursMode();
}

// static
// Return ChromeDeviceSettingsProto without |ignored_policies|.
// |ignored_policies| will be apply with default values.
std::unique_ptr<em::ChromeDeviceSettingsProto>
DeviceOffHoursController::GetOffHoursProto(
    const em::ChromeDeviceSettingsProto& input_policies) {
  const em::DeviceOffHoursProto& container(input_policies.device_off_hours());
  std::vector<std::string> ignored_policies = GetIgnoredPolicies(container);
  em::ChromeDeviceSettingsProto policies;
  policies.CopyFrom(input_policies);
  RemovePolicies(&policies, ignored_policies);
  return base::MakeUnique<em::ChromeDeviceSettingsProto>(policies);
}

}  // namespace policy
