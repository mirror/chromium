// Copyright (c) 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/chromeos/policy/device_off_hours_controller.h"

#include <utility>

#include "chrome/browser/chromeos/policy/device_policy_cros_browser_test.h"
#include "chrome/browser/chromeos/policy/proto/chrome_device_policy.pb.h"
#include "chrome/browser/chromeos/settings/cros_settings.h"
#include "chrome/test/remoting/waiter.h"

namespace em = enterprise_management;

namespace {
static std::vector<std::string> kDefaultStringVector{"allow_new_users",
                                                     "guest_mode_enabled"};
static std::vector<em::DeviceOffHoursIntervalProto> kDefaultIntervalVector;
static em::WeeklyTimeProto_Weekday weekdays[] = {
    em::WeeklyTimeProto::MONDAY,   em::WeeklyTimeProto::MONDAY,
    em::WeeklyTimeProto::TUESDAY,  em::WeeklyTimeProto::WEDNESDAY,
    em::WeeklyTimeProto::THURSDAY, em::WeeklyTimeProto::FRIDAY,
    em::WeeklyTimeProto::SATURDAY, em::WeeklyTimeProto::SUNDAY};
const int kMillisecondsInMinute =
    base::TimeDelta::FromMinutes(1).InMilliseconds();
const int kMillisecondsInHour = base::TimeDelta::FromHours(1).InMilliseconds();
const int kMillisecondsInDay = base::TimeDelta::FromDays(1).InMilliseconds();
const int kMillisecondsInWeek = base::TimeDelta::FromDays(7).InMilliseconds();

}  // namespace

namespace policy {

class DeviceOffHoursPolicyControllerTest
    : public policy::DevicePolicyCrosBrowserTest {
 public:
  DeviceOffHoursPolicyControllerTest() {}

  void SetUpInProcessBrowserTestFixture() override {
    InstallOwnerKey();
    MarkAsEnterpriseOwned();
    DevicePolicyCrosBrowserTest::SetUpInProcessBrowserTestFixture();
  }

 protected:
  void RefreshPolicyAndWaitUntilDeviceSettingsUpdated(const char* device_key) {
    base::RunLoop run_loop;
    std::unique_ptr<chromeos::CrosSettings::ObserverSubscription> observer =
        chromeos::CrosSettings::Get()->AddSettingsObserver(
            device_key, run_loop.QuitClosure());
    RefreshDevicePolicy();
    run_loop.Run();
  }

  void SetPolicy(
      const std::vector<em::DeviceOffHoursIntervalProto>& intervals,
      const std::string& timezone = "GMT",
      const std::vector<std::string>& ignored_policy = kDefaultStringVector) {
    em::ChromeDeviceSettingsProto& proto(device_policy()->payload());
    auto* off_hours = proto.mutable_device_off_hours();
    for (auto interval : intervals) {
      auto* cur = off_hours->add_interval();
      *cur = interval;
    }
    off_hours->set_timezone(timezone);
    for (auto p : ignored_policy) {
      off_hours->add_ignored_policy(p);
    }
    RefreshPolicyAndWaitUntilDeviceSettingsUpdated(chromeos::kDeviceOffHours);
  }

  void UnsetPolicy() {
    em::ChromeDeviceSettingsProto& proto(device_policy()->payload());
    proto.clear_device_off_hours();
    RefreshPolicyAndWaitUntilDeviceSettingsUpdated(chromeos::kDeviceOffHours);
  }

  em::DeviceOffHoursIntervalProto createInterval(int st_w,
                                                 int st_ms,
                                                 int en_w,
                                                 int en_ms) {
    em::DeviceOffHoursIntervalProto interval =
        em::DeviceOffHoursIntervalProto();
    em::WeeklyTimeProto* start = interval.mutable_start();
    em::WeeklyTimeProto* end = interval.mutable_end();
    start->set_weekday(weekdays[st_w]);
    start->set_time(st_ms);
    end->set_weekday(weekdays[en_w]);
    end->set_time(en_ms);
    return interval;
  }

  int toMilliseconds(int hours, int minutes) {
    return (hours * 60 + minutes) * 60 * 1000;
  }

  // delay, duration in milliseconds
  void CreateAndSetOffHoursFromNow(int delay = 0,
                                   int duration = kMillisecondsInHour) {
    const time_t theTime = base::Time::Now().ToTimeT();
    struct tm* aTime = gmtime(&theTime);
    int cur_w = aTime->tm_wday;
    int cur_ms = (aTime->tm_hour * 60 + aTime->tm_min) * 60 * 1000;
    int start_ms = (cur_ms + delay) % kMillisecondsInDay;
    int deltaW = (cur_ms + delay) / kMillisecondsInDay;
    int start_w = (cur_w + deltaW - 1) % 7 + 1;
    int end_ms = (start_ms + duration) % kMillisecondsInDay;
    deltaW = (start_ms + duration) / kMillisecondsInDay;
    int end_w = (start_w + deltaW - 1) % 7 + 1;
    std::vector<em::DeviceOffHoursIntervalProto> intervals;
    LOG(ERROR) << "Daria: " << start_w << " " << start_ms << " " << end_w << " "
               << end_ms;
    intervals.push_back(createInterval(start_w, start_ms, end_w, end_ms));
    SetPolicy(intervals);
  }
};

IN_PROC_BROWSER_TEST_F(DeviceOffHoursPolicyControllerTest, CheckUnset) {
  const base::DictionaryValue* off_hours = nullptr;
  EXPECT_FALSE(chromeos::CrosSettings::Get()->GetDictionary(
      chromeos::kDeviceOffHours, &off_hours));

  enterprise_management::ChromeDeviceSettingsProto& proto(
      device_policy()->payload());
  // guest_mode_enabled = 1 [default = true]
  // data_roaming_enabled = 1 [default = false]
  proto.mutable_guest_mode_enabled()->set_guest_mode_enabled(false);
  proto.mutable_data_roaming_enabled()->set_data_roaming_enabled(true);
  RefreshPolicyAndWaitUntilDeviceSettingsUpdated(
      chromeos::kAccountsPrefAllowGuest);
  bool guest_mode_enabled = true;
  EXPECT_TRUE(chromeos::CrosSettings::Get()->GetBoolean(
      chromeos::kAccountsPrefAllowGuest, &guest_mode_enabled));
  EXPECT_FALSE(guest_mode_enabled);
  bool data_roaming_enabled = false;
  EXPECT_TRUE(chromeos::CrosSettings::Get()->GetBoolean(
      chromeos::kSignedDataRoamingEnabled, &data_roaming_enabled));
  EXPECT_TRUE(data_roaming_enabled);
}

IN_PROC_BROWSER_TEST_F(DeviceOffHoursPolicyControllerTest, CheckSet) {
  enterprise_management::ChromeDeviceSettingsProto& proto(
      device_policy()->payload());
  // guest_mode_enabled = 1 [default = true]
  proto.mutable_guest_mode_enabled()->set_guest_mode_enabled(false);
  proto.mutable_data_roaming_enabled()->set_data_roaming_enabled(true);
  RefreshPolicyAndWaitUntilDeviceSettingsUpdated(
      chromeos::kAccountsPrefAllowGuest);

  const base::DictionaryValue* off_hours = nullptr;
  EXPECT_FALSE(chromeos::CrosSettings::Get()->GetDictionary(
      chromeos::kDeviceOffHours, &off_hours));
  CreateAndSetOffHoursFromNow(kMillisecondsInHour, kMillisecondsInHour);
  EXPECT_TRUE(chromeos::CrosSettings::Get()->GetDictionary(
      chromeos::kDeviceOffHours, &off_hours));
  const base::ListValue* intervals = nullptr;
  const base::ListValue* ignored_policies = nullptr;
  std::string timezone;
  EXPECT_TRUE(off_hours->GetList("intervals", &intervals));
  EXPECT_TRUE(off_hours->GetList("ignored_policies", &ignored_policies));
  EXPECT_TRUE(off_hours->GetString("timezone", &timezone));
  bool guest_mode_enabled = true;
  EXPECT_TRUE(chromeos::CrosSettings::Get()->GetBoolean(
      chromeos::kAccountsPrefAllowGuest, &guest_mode_enabled));
  EXPECT_FALSE(guest_mode_enabled);
  bool data_roaming_enabled = false;
  EXPECT_TRUE(chromeos::CrosSettings::Get()->GetBoolean(
      chromeos::kSignedDataRoamingEnabled, &data_roaming_enabled));
  EXPECT_TRUE(data_roaming_enabled);
}

IN_PROC_BROWSER_TEST_F(DeviceOffHoursPolicyControllerTest, SetOffHoursModeNow) {
  enterprise_management::ChromeDeviceSettingsProto& proto(
      device_policy()->payload());
  // guest_mode_enabled = 1 [default = true]
  proto.mutable_guest_mode_enabled()->set_guest_mode_enabled(false);
  proto.mutable_data_roaming_enabled()->set_data_roaming_enabled(true);
  RefreshPolicyAndWaitUntilDeviceSettingsUpdated(
      chromeos::kAccountsPrefAllowGuest);

  bool guest_mode_enabled = true;
  EXPECT_TRUE(chromeos::CrosSettings::Get()->GetBoolean(
      chromeos::kAccountsPrefAllowGuest, &guest_mode_enabled));
  EXPECT_FALSE(guest_mode_enabled);

  CreateAndSetOffHoursFromNow();
  EXPECT_TRUE(chromeos::CrosSettings::Get()->GetBoolean(
      chromeos::kAccountsPrefAllowGuest, &guest_mode_enabled));
  EXPECT_TRUE(guest_mode_enabled);
  bool data_roaming_enabled = false;
  EXPECT_TRUE(chromeos::CrosSettings::Get()->GetBoolean(
      chromeos::kSignedDataRoamingEnabled, &data_roaming_enabled));
  EXPECT_TRUE(data_roaming_enabled);
}
}  // namespace policy
