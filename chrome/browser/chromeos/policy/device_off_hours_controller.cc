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
#include "chrome/browser/chromeos/settings/cros_settings.h"
#include "chrome/browser/chromeos/system/timezone_util.h"
#include "chrome/browser/lifetime/application_lifetime.h"
#include "chromeos/dbus/dbus_thread_manager.h"
#include "chromeos/dbus/device_off_hours_client.h"
#include "chromeos/login/login_state.h"
#include "chromeos/settings/timezone_settings.h"
#include "components/policy/core/common/cloud/cloud_policy_store.h"
#include "components/user_manager/user_manager.h"
#include "unicode/gregocal.h"

#include "base/i18n/rtl.h"
#include "base/i18n/time_formatting.h"
#include "base/i18n/unicodestring.h"
#include "base/strings/string16.h"
#include "google_apis/gaia/gaia_auth_util.h"
#include "third_party/icu/source/i18n/unicode/datefmt.h"

namespace em = enterprise_management;

namespace policy {
namespace {

const int kMillisecondsInMinute =
    base::TimeDelta::FromMinutes(1).InMilliseconds();
const int kMillisecondsInHour = base::TimeDelta::FromHours(1).InMilliseconds();
const int kMillisecondsInDay = base::TimeDelta::FromDays(1).InMilliseconds();
const int kMillisecondsInWeek = base::TimeDelta::FromDays(7).InMilliseconds();

void DaylightTimeChangedDate(const std::string& timezone) {
  icu::TimeZone* zone =
      icu::TimeZone::createTimeZone(icu::UnicodeString::fromUTF8(timezone));
  UErrorCode status = U_ZERO_ERROR;
  icu::GregorianCalendar* gc = new icu::GregorianCalendar(zone, status);
  status = U_ZERO_ERROR;
  UBool daylight = gc->inDaylightTime(status);
  LOG(ERROR) << "!!! cur daylight=" << (bool)daylight;
  for (int i = 0; i < 365; i++) {
    UDate cur_date = icu::Calendar::getNow() + (double)kMillisecondsInDay * i;
    status = U_ZERO_ERROR;
    gc->setTime(cur_date, status);
    if (U_FAILURE(status)) {
      LOG(ERROR) << "!!! Calendar set date error = " << u_errorName(status);
    }
    status = U_ZERO_ERROR;
    UBool cur_daylight = gc->inDaylightTime(status);
    if (cur_daylight != daylight) {
      daylight = cur_daylight;
      icu::UnicodeString date_string;
      icu::DateFormat* formatter =
          icu::DateFormat::createDateTimeInstance(icu::DateFormat::kFull);
      formatter->format(gc->getTime(status), date_string);
      LOG(ERROR) << "!!! DAYLIGHT CHANGE IN "
                 << base::i18n::UnicodeStringToString16(date_string);
    }
  }
}

// WeeklyTime class contains weekday and time in milliseconds.
class WeeklyTime {
 public:
  WeeklyTime(int day_of_week, int milliseconds)
      : day_of_week_(day_of_week), milliseconds_(milliseconds) {}

  // Constructor converts input time in timezone |timezone|
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
    LOG(ERROR) << "Daria: "
               << "DST=" << delta_DST;
    UErrorCode status = U_ZERO_ERROR;
    icu::GregorianCalendar* gc = new icu::GregorianCalendar(zone, status);
    LOG(ERROR) << "Daria: "
               << "calendar status=" << u_errorName(status);
    status = U_ZERO_ERROR;
    UBool day_light = gc->inDaylightTime(status);
    if (U_FAILURE(status)) {
      LOG(ERROR) << "Daylight time error = " << u_errorName(status);
    }
    LOG(ERROR) << "Daria: daylight: " << (bool)day_light
               << "; status=" << u_errorName(status)
               << ", timezone=" << timezone;
    if (day_light) {
      LOG(ERROR) << "Daria: Day Light Time!! timezone = " << timezone;
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
  // Number of day_of_week (1 = Monday, 2 = Tuesday, etc.)
  int day_of_week_;
  // Time of day in milliseconds from the beginning of the day.
  int milliseconds_;
};

// Time interval struct, interval = [start, end].
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

// Check if WeeklyTime |w| in interval |interval|
bool InInterval(const Interval& interval, const WeeklyTime& w) {
  if (interval.start == interval.end) {
    if (interval.start == w) {
      return true;
    }
  } else if (interval.start < interval.end) {
    if (w >= interval.start && w <= interval.end) {
      return true;
    }
  } else {
    if (w >= interval.start || w <= interval.end) {
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

// Go through interval and check if "OffHours" mode is in current time.
bool CheckOffHoursMode(const std::vector<Interval>& intervals) {
  base::Time::Exploded exploded;
  base::Time cur_time = base::Time::Now();
  base::TimeTicks start = base::TimeTicks::Now();
  cur_time.UTCExplode(&exploded);
  WeeklyTime cur_t(exploded.day_of_week,
                   exploded.hour * kMillisecondsInHour +
                       exploded.minute * kMillisecondsInMinute +
                       exploded.second * 1000);
  LOG(ERROR) << "Daria CURRENT TIME UTC:" << cur_t.ToString();
  int before_off_hours = kMillisecondsInWeek;
  for (size_t i = 0; i < intervals.size(); i++) {
    LOG(ERROR) << "Daria interval " << intervals[i].ToString();
    if (InInterval(intervals[i], cur_t)) {
      int duration = intervals[i].end - cur_t + kMillisecondsInMinute;
      DeviceOffHoursController::Get()->SetOffHoursStartTime(start);
      DeviceOffHoursController::Get()->SetOffHoursDuration(
          base::TimeDelta::FromMilliseconds(duration));
      DeviceOffHoursController::Get()->SetOffHoursMode();
      DeviceOffHoursController::Get()->StartOffHoursTimer(duration);
      return true;
    }
    before_off_hours = std::min(before_off_hours, intervals[i].start - cur_t);
  }
  if (before_off_hours < kMillisecondsInWeek) {
    DeviceOffHoursController::Get()->StartOffHoursTimer(before_off_hours);
  }
  DeviceOffHoursController::Get()->UnsetOffHoursMode();
  return false;
}

// Check if current user is allowed based on settings in |device_policies|.
// Checked policies: guest_mode_enabled, allow_new_users, user_whitelist.
bool IsCurrentUserAllowed(
    const em::ChromeDeviceSettingsProto& device_policies) {
  LOG(ERROR) << "Daria: IsUserAllowed()";
  user_manager::UserManager* user_manager = user_manager::UserManager::Get();
  bool is_guest = user_manager->IsLoggedInAsGuest();
  if (is_guest) {
    LOG(ERROR) << "Daria: guest mode now";
    if (device_policies.has_guest_mode_enabled()) {
      const em::GuestModeEnabledProto& container(
          device_policies.guest_mode_enabled());
      if (container.has_guest_mode_enabled() &&
          !container.guest_mode_enabled()) {
        LOG(ERROR) << "Daria: guest mode turn off, current user isn't allowed";
        return false;
      }
    }
    return true;
  }
  if (user_manager->IsLoggedInAsPublicAccount() ||
      user_manager->IsLoggedInAsSupervisedUser() ||
      user_manager->IsLoggedInAsKioskApp() ||
      user_manager->IsLoggedInAsArcKioskApp()) {
    return true;
  }
  const std::string current_user =
      user_manager->GetActiveUser()->GetAccountId().GetUserEmail();
  LOG(ERROR) << "Daria: current email = " << current_user;
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
    if (allow_new_users && whitelist->empty())
      return true;
    bool user_in_whitelist = chromeos::CrosSettings::Get()->FindEmailInList(
        whitelist, current_user, NULL);
    if (!user_in_whitelist)
      return false;
  }
  return true;
}

em::ChromeDeviceSettingsProto* device_policies = NULL;

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
  chromeos::DBusThreadManager::Get()->GetSessionManagerClient()->AddObserver(
      this);
  chromeos::DBusThreadManager::Get()->GetSystemClockClient()->AddObserver(this);
}

DeviceOffHoursController::~DeviceOffHoursController() {
  for (auto& observer : observers_)
    observer.OffHoursControllerShutDown();
  chromeos::DBusThreadManager::Get()->GetSessionManagerClient()->RemoveObserver(
      this);
  chromeos::DBusThreadManager::Get()->GetSystemClockClient()->RemoveObserver(
      this);
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
  if (off_hours_mode_)
    return;
  LOG(ERROR) << "Daria Off hours mode set";
  off_hours_mode_ = true;
  NotifyOffHoursModeUploaded();
}

void DeviceOffHoursController::UnsetOffHoursMode() {
  if (!off_hours_mode_) {
    return;
  }
  off_hours_mode_ = false;
  SetOffHoursDuration(base::TimeDelta::FromMilliseconds(0));
  NotifyOffHoursModeUploaded();
  LogoutUserIfNeed(*device_policies);
}

void DeviceOffHoursController::SetOffHoursStartTime(
    base::TimeTicks off_hours_start) {
  off_hours_start_ = off_hours_start;
}

void DeviceOffHoursController::SetOffHoursDuration(
    base::TimeDelta off_hours_duration) {
  off_hours_duration_ = off_hours_duration;
}

// Timer
// |delay| value in milliseconds.
void DeviceOffHoursController::StartOffHoursTimer(int delay) {
  LOG(ERROR) << "Daria Timer started with delay(minutes) = "
             << delay / kMillisecondsInMinute;
  if (delay <= 0) {
    LOG(ERROR) << "Daria ???? delay=" << delay;
    return;
  }
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
  NotifyOffHoursModeUploaded();
  LOG(ERROR) << "Daria Timer ok";
}

void DeviceOffHoursController::StopOffHoursTimer() {
  timer_.Stop();
}

void DeviceOffHoursController::NotifyOffHoursModeChanged() const {
  if (!IsInitialized()) {
    return;
  }
  LOG(ERROR) << "Daria NotifyOffHoursModeChanged";
  for (auto& observer : observers_)
    observer.OffHoursModeChanged();
}

void DeviceOffHoursController::NotifyOffHoursModeUploaded() const {
  if (!IsInitialized()) {
    return;
  }
  LOG(ERROR) << "Daria NotifyOffHoursModeUploaded";
  for (auto& observer : observers_)
    observer.OffHoursModeUploaded();
}

void DeviceOffHoursController::ScreenIsUnlocked() {
  LOG(ERROR) << "Daria screen is unlocked";
  NotifyOffHoursModeChanged();
}

void DeviceOffHoursController::SystemClockUpdated() {
  LOG(ERROR) << "Daria system clock updated";
  NotifyOffHoursModeChanged();
}

// Call chrome::AttemptUserExit()
// if guest mode or current user aren't enabled more.
void DeviceOffHoursController::LogoutUserIfNeed(
    const em::ChromeDeviceSettingsProto& policies) {
  LOG(ERROR) << "Daria: LogoutUserIfNeed()";
  bool is_log_in = chromeos::LoginState::Get()->IsUserLoggedIn();
  if (!is_log_in) {
    LOG(ERROR) << "Daria: not log in";
    return;
  }
  if (!IsCurrentUserAllowed(policies)) {
    LOG(ERROR) << "Daria: user isn't allowed, attempt user exit";
    VLOG(1) << "User isn't allowed. Will attempt user exit.";
    chrome::AttemptUserExit();
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

  chromeos::DeviceOffHoursClient* device_off_hours_client =
      chromeos::DBusThreadManager::Get()->GetDeviceOffHoursClient();
  if (!device_off_hours_client->IsNetworkSyncronized()) {
    LOG(ERROR) << "Network isn't syncronized. OffHours mode is not available.";
    return nullptr;
  }

  device_policies = input_policies;
  LOG(ERROR) << "Daria OFF HOURS POLICIES " << Get()->IsOffHoursMode();
  std::vector<Interval> intervals =
      GetIntervals(container, container.timezone());
  LOG(ERROR) << "Daria: INTERVAL COUNT " << intervals.size();

  DaylightTimeChangedDate(container.timezone());

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
  RemovePolicies(&policies, ignored_policies);
  return base::MakeUnique<em::ChromeDeviceSettingsProto>(policies);
}

}  // namespace policy
