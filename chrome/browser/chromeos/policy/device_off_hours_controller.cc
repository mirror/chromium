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
           std::to_string((milliseconds_ % kMillisecondsInHour) /
                          kMillisecondsInMinute);
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
  return base::MakeUnique<WeeklyTime>(container.weekday(), time_of_day,
                                      timezone);
}

// Get and return list of time intervals from DeviceOffHoursProto structure.
std::vector<Interval> GetIntervals(const em::DeviceOffHoursProto& container,
                                   const std::string& timezone = "UTC") {
  std::vector<Interval> intervals;
  for (const auto& entry : container.interval()) {
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
  return std::vector<std::string>(container.ignored_policy().begin(),
                                  container.ignored_policy().end());
}

// Go through interval and check if "OffHours" mode is in current time.
bool CheckOffHoursMode(const std::vector<Interval>& intervals) {
  const time_t theTime = base::Time::Now().ToTimeT();
  struct tm* aTime = gmtime(&theTime);
  WeeklyTime cur_t(aTime->tm_wday, (aTime->tm_hour * 60 + aTime->tm_min) *
                                       kMillisecondsInMinute);
  LOG(ERROR) << "Daria CURRENT TIME UTC:" << cur_t.ToString();
  int before_off_hours = kMillisecondsInWeek;
  for (size_t i = 0; i < intervals.size(); i++) {
    LOG(ERROR) << "Daria interval " << intervals[i].ToString();
    if (InInterval(intervals[i], cur_t)) {
      DeviceOffHoursController::Get()->SetOffHoursMode();
      DeviceOffHoursController::Get()->StartOffHoursTimer(
          intervals[i].end - cur_t + kMillisecondsInMinute);
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
  LOG(ERROR) << "Daria Off hours mode set";
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
  LOG(ERROR) << "Daria Timer ok";
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
  LOG(ERROR) << "Daria: LogoutUserIfNeed()";
  bool is_log_in = chromeos::LoginState::Get()->IsUserLoggedIn();
  bool is_guest = chromeos::LoginState::Get()->IsGuestSessionUser();
  if (is_guest) {
    LOG(ERROR) << "Daria: guest mode now";
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
    LOG(ERROR) << "Daria: not log in";
    return;
  }
  const std::string current_user = user_manager::UserManager::Get()
                                       ->GetActiveUser()
                                       ->GetAccountId()
                                       .GetUserEmail();
  LOG(ERROR) << "Daria: current email = " << current_user;
  if (device_policies->has_user_whitelist()) {
    const em::UserWhitelistProto& container(device_policies->user_whitelist());
    bool user_is_allowed = false;

    std::string canonicalized_email(
        gaia::CanonicalizeEmail(gaia::SanitizeEmail(current_user)));
    std::string wildcard_email;
    std::string::size_type at_pos = canonicalized_email.find('@');
    if (at_pos != std::string::npos) {
      wildcard_email =
          std::string("*").append(canonicalized_email.substr(at_pos));
    }
    for (const std::string& entry : container.user_whitelist()) {
      std::string canonicalized_entry(
          gaia::CanonicalizeEmail(gaia::SanitizeEmail(entry)));
      LOG(ERROR) << "Daria: whitelist = " << canonicalized_entry;
      if (canonicalized_entry != wildcard_email &&
          canonicalized_entry == canonicalized_email) {
        user_is_allowed = true;
      } else if (canonicalized_entry == wildcard_email) {
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
