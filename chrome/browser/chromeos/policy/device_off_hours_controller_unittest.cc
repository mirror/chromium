// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/chromeos/policy/device_off_hours_controller.h"

#include <utility>

#include "base/memory/ptr_util.h"
#include "base/time/default_clock.h"
#include "base/time/tick_clock.h"
#include "base/time/time.h"
#include "base/values.h"
#include "chrome/browser/chromeos/policy/proto/chrome_device_policy.pb.h"
#include "chrome/browser/chromeos/settings/device_settings_test_helper.h"
#include "chromeos/dbus/dbus_thread_manager.h"
#include "chromeos/dbus/fake_power_manager_client.h"
#include "chromeos/dbus/fake_system_clock_client.h"
#include "testing/gmock/include/gmock/gmock.h"

namespace em = enterprise_management;

namespace chromeos {

using policy::off_hours::OffHoursInterval;
using policy::off_hours::WeeklyTime;

namespace {

constexpr em::WeeklyTimeProto_DayOfWeek kWeekdays[] = {
    em::WeeklyTimeProto::DAY_OF_WEEK_UNSPECIFIED,
    em::WeeklyTimeProto::MONDAY,
    em::WeeklyTimeProto::TUESDAY,
    em::WeeklyTimeProto::WEDNESDAY,
    em::WeeklyTimeProto::THURSDAY,
    em::WeeklyTimeProto::FRIDAY,
    em::WeeklyTimeProto::SATURDAY,
    em::WeeklyTimeProto::SUNDAY};

constexpr base::TimeDelta kHour = base::TimeDelta::FromHours(1);
constexpr base::TimeDelta kDay = base::TimeDelta::FromDays(1);

class FakeClock : public base::Clock {
 public:
  FakeClock() { fake_now_ = base::Time::Now(); }

  base::Time Now() override { return fake_now_; }

  void SetFakeTimeNow(base::Time now) { fake_now_ = now; }

 private:
  base::Time fake_now_;
};

class FakeTickClock : public base::TickClock {
 public:
  FakeTickClock() { fake_now_ticks_ = base::TimeTicks::Now(); }

  base::TimeTicks NowTicks() override { return fake_now_ticks_; }

  void SetFakeTimeTicksNow(base::TimeTicks now) { fake_now_ticks_ = now; }

 private:
  base::TimeTicks fake_now_ticks_;
};

void RemoveOffHoursPolicyFromProto(em::ChromeDeviceSettingsProto& proto) {
  proto.clear_device_off_hours();
}

void SetOffHoursPolicyToProto(
    em::ChromeDeviceSettingsProto& proto,
    const std::vector<em::DeviceOffHoursIntervalProto>& intervals,
    const std::string& timezone,
    const std::vector<int>& ignored_policy_proto_tags) {
  RemoveOffHoursPolicyFromProto(proto);
  auto* off_hours = proto.mutable_device_off_hours();
  for (auto interval : intervals) {
    auto* cur = off_hours->add_intervals();
    *cur = interval;
  }
  off_hours->set_timezone(timezone);
  for (auto p : ignored_policy_proto_tags) {
    off_hours->add_ignored_policy_proto_tags(p);
  }
}

em::DeviceOffHoursIntervalProto CreateOffHoursIntervalProto(
    int start_day_of_week,
    base::TimeDelta start_time,
    int end_day_of_week,
    base::TimeDelta end_time) {
  em::DeviceOffHoursIntervalProto interval = em::DeviceOffHoursIntervalProto();
  em::WeeklyTimeProto* start = interval.mutable_start();
  em::WeeklyTimeProto* end = interval.mutable_end();
  start->set_day_of_week(kWeekdays[start_day_of_week]);
  start->set_time(start_time.InMilliseconds());
  end->set_day_of_week(kWeekdays[end_day_of_week]);
  end->set_time(end_time.InMilliseconds());
  return interval;
}

// Create and set "OffHours" interval which starts in |delay| time and with
// |duration|. Zero delay means that "OffHours" mode start right now.
void CreateAndSetOffHoursPolicyToProto(em::ChromeDeviceSettingsProto& proto,
                                       base::TimeDelta delay,
                                       base::TimeDelta duration) {
  base::Time::Exploded exploded;
  base::Time::Now().UTCExplode(&exploded);
  int cur_day_of_week = exploded.day_of_week;
  if (cur_day_of_week == 0)
    cur_day_of_week = 7;
  base::TimeDelta the_time = base::TimeDelta::FromDays(cur_day_of_week - 1) +
                             base::TimeDelta::FromHours(exploded.hour) +
                             base::TimeDelta::FromMinutes(exploded.minute) +
                             base::TimeDelta::FromSeconds(exploded.second);
  the_time += delay;
  base::TimeDelta start_time = the_time % kDay;
  int start_day_of_week = the_time.InDays() % 7 + 1;
  the_time += duration;
  base::TimeDelta end_time = the_time % kDay;
  int end_day_of_week = the_time.InDays() % 7 + 1;

  std::vector<em::DeviceOffHoursIntervalProto> intervals;
  intervals.push_back(CreateOffHoursIntervalProto(start_day_of_week, start_time,
                                                  end_day_of_week, end_time));
  std::vector<int> ignored_policy_proto_tags = {
      3 /*DeviceGuestModeEnabled policy*/};
  SetOffHoursPolicyToProto(proto, intervals, "GMT", ignored_policy_proto_tags);
}

}  // namespace

class DeviceOffHoursControllerTest : public DeviceSettingsTestBase {
 protected:
  DeviceOffHoursControllerTest() {}

  void SetUp() override {
    DeviceSettingsTestBase::SetUp();
    system_clock_client_ = new chromeos::FakeSystemClockClient();
    dbus_setter_->SetSystemClockClient(base::WrapUnique(system_clock_client_));
    device_settings_service_.SetDeviceOffHoursControllerForTesting(
        base::MakeUnique<policy::DeviceOffHoursController>());
  }

  void SetFakeFromNow(base::TimeDelta duration) {
    fake_clock_->SetFakeTimeNow(base::Time::Now() + duration);
    fake_tick_clock_->SetFakeTimeTicksNow(base::TimeTicks::Now() + duration);
  }

  void UpdateDeviceSettings() {
    device_policy_.Build();
    device_settings_test_helper_.set_policy_blob(device_policy_.GetBlob());
    ReloadDeviceSettings();
  }

  base::Clock* GetClock() { return fake_clock_; }

  base::TickClock* GetTickClock() { return fake_tick_clock_; }

  chromeos::FakeSystemClockClient* system_clock_client() {
    return system_clock_client_;
  }

 private:
  FakeClock* fake_clock_ = new FakeClock();
  FakeTickClock* fake_tick_clock_ = new FakeTickClock();

  // The object is owned by DeviceSettingsTestBase class.
  chromeos::FakeSystemClockClient* system_clock_client_;

  DISALLOW_COPY_AND_ASSIGN(DeviceOffHoursControllerTest);
};

TEST_F(DeviceOffHoursControllerTest, CheckOffHoursUnset) {
  system_clock_client()->set_network_synchronized(true);
  system_clock_client()->NotifyObserversSystemClockUpdated();
  em::ChromeDeviceSettingsProto& proto(device_policy_.payload());
  proto.mutable_guest_mode_enabled()->set_guest_mode_enabled(false);
  UpdateDeviceSettings();
  EXPECT_FALSE(device_settings_service_.device_settings()
                   ->guest_mode_enabled()
                   .guest_mode_enabled());
  RemoveOffHoursPolicyFromProto(proto);
  UpdateDeviceSettings();
  EXPECT_FALSE(device_settings_service_.device_settings()
                   ->guest_mode_enabled()
                   .guest_mode_enabled());
}

TEST_F(DeviceOffHoursControllerTest, CheckOffHoursModeOff) {
  system_clock_client()->set_network_synchronized(true);
  system_clock_client()->NotifyObserversSystemClockUpdated();
  em::ChromeDeviceSettingsProto& proto(device_policy_.payload());
  proto.mutable_guest_mode_enabled()->set_guest_mode_enabled(false);
  UpdateDeviceSettings();
  EXPECT_FALSE(device_settings_service_.device_settings()
                   ->guest_mode_enabled()
                   .guest_mode_enabled());
  CreateAndSetOffHoursPolicyToProto(proto, kHour, kHour);
  UpdateDeviceSettings();
  EXPECT_FALSE(device_settings_service_.device_settings()
                   ->guest_mode_enabled()
                   .guest_mode_enabled());
}

TEST_F(DeviceOffHoursControllerTest, CheckOffHoursModeOn) {
  system_clock_client()->set_network_synchronized(true);
  system_clock_client()->NotifyObserversSystemClockUpdated();
  em::ChromeDeviceSettingsProto& proto(device_policy_.payload());
  proto.mutable_guest_mode_enabled()->set_guest_mode_enabled(false);
  UpdateDeviceSettings();
  EXPECT_FALSE(device_settings_service_.device_settings()
                   ->guest_mode_enabled()
                   .guest_mode_enabled());
  CreateAndSetOffHoursPolicyToProto(proto, base::TimeDelta(), kHour);
  UpdateDeviceSettings();
  EXPECT_TRUE(device_settings_service_.device_settings()
                  ->guest_mode_enabled()
                  .guest_mode_enabled());
}

TEST_F(DeviceOffHoursControllerTest, NoNetworkSynchronization) {
  system_clock_client()->set_network_synchronized(false);
  system_clock_client()->NotifyObserversSystemClockUpdated();
  enterprise_management::ChromeDeviceSettingsProto& proto(
      device_policy_.payload());
  proto.mutable_guest_mode_enabled()->set_guest_mode_enabled(false);
  UpdateDeviceSettings();
  EXPECT_FALSE(device_settings_service_.device_settings()
                   ->guest_mode_enabled()
                   .guest_mode_enabled());
  CreateAndSetOffHoursPolicyToProto(proto, base::TimeDelta(), kHour);
  EXPECT_FALSE(device_settings_service_.device_settings()
                   ->guest_mode_enabled()
                   .guest_mode_enabled());
}

TEST_F(DeviceOffHoursControllerTest, GetTimezoneOffset) {
  //  FakeClock* clock = new FakeClock();
  // TODO(yakovleva): how tests anonymous namespace?
}

TEST_F(DeviceOffHoursControllerTest, ConvertOffHoursProtoToValue) {
  WeeklyTime start = WeeklyTime(1, kHour.InMilliseconds());
  WeeklyTime end = WeeklyTime(3, kHour.InMilliseconds() * 2);
  std::vector<em::DeviceOffHoursIntervalProto> intervals_proto = {
      CreateOffHoursIntervalProto(1, kHour, 3, kHour * 2)};
  std::vector<OffHoursInterval> intervals = {OffHoursInterval(start, end)};
  std::string timezone = "Germany/Berlin";
  std::vector<int> ignored_policy_proto_tags = {3, 8};
  em::ChromeDeviceSettingsProto proto = em::ChromeDeviceSettingsProto();
  SetOffHoursPolicyToProto(proto, intervals_proto, timezone,
                           ignored_policy_proto_tags);

  std::unique_ptr<base::DictionaryValue> off_hours_value =
      policy::ConvertOffHoursProtoToValue(proto.device_off_hours());

  base::DictionaryValue off_hours_expected;
  off_hours_expected.SetString("timezone", timezone);
  auto intervals_value = base::MakeUnique<base::ListValue>();
  for (const auto& interval : intervals)
    intervals_value->Append(interval.ToValue());
  off_hours_expected.SetList("intervals", std::move(intervals_value));
  auto ignored_policies_value = base::MakeUnique<base::ListValue>();
  for (const auto& policy : ignored_policy_proto_tags)
    ignored_policies_value->GetList().emplace_back(policy);
  off_hours_expected.SetList("ignored_policy_proto_tags",
                             std::move(ignored_policies_value));

  EXPECT_EQ(*off_hours_value, off_hours_expected);
}

TEST_F(DeviceOffHoursControllerTest, ApplyOffHoursPolicyToProto) {
  em::ChromeDeviceSettingsProto proto = em::ChromeDeviceSettingsProto();
  proto.mutable_guest_mode_enabled()->set_guest_mode_enabled(false);
  proto.mutable_allow_new_users()->set_allow_new_users(false);
  proto.mutable_camera_enabled()->set_camera_enabled(false);
  // proto tag = 3 is DeviceAllowNewUsers policy
  // proto tag = 8 is the DeviceGuestModeEnabled policy
  SetOffHoursPolicyToProto(proto, {}, "GMT", {3, 8});
  std::unique_ptr<em::ChromeDeviceSettingsProto> off_hours_proto =
      policy::ApplyOffHoursPolicyToProto(proto);
  EXPECT_TRUE(off_hours_proto->guest_mode_enabled().guest_mode_enabled());
  EXPECT_TRUE(off_hours_proto->allow_new_users().allow_new_users());
  EXPECT_FALSE(off_hours_proto->camera_enabled().camera_enabled());
}

TEST_F(DeviceOffHoursControllerTest, FakeClock) {
  device_settings_service_.SetDeviceOffHoursControllerForTesting(
      base::MakeUnique<policy::DeviceOffHoursController>(GetClock(),
                                                         GetTickClock()));
  system_clock_client()->set_network_synchronized(true);
  system_clock_client()->NotifyObserversSystemClockUpdated();
  policy::DeviceOffHoursController* device_off_hours_controller =
      device_settings_service_.device_off_hours_controller();
  EXPECT_FALSE(device_off_hours_controller->is_off_hours_mode());
  em::ChromeDeviceSettingsProto proto = em::ChromeDeviceSettingsProto();
  CreateAndSetOffHoursPolicyToProto(proto, base::TimeDelta(), kHour);
  device_off_hours_controller->UpdateOffHoursPolicy(proto);
  EXPECT_TRUE(device_off_hours_controller->is_off_hours_mode());
  SetFakeFromNow(base::TimeDelta::FromHours(2));
  device_off_hours_controller->UpdateOffHoursPolicy(proto);
  EXPECT_FALSE(device_off_hours_controller->is_off_hours_mode());
}

class OneOffHoursIntervalTest
    : public DeviceOffHoursControllerTest,
      public testing::WithParamInterface<
          std::tuple<base::TimeDelta, base::TimeDelta, base::TimeDelta, bool>> {
 public:
  base::TimeDelta start_in() const { return std::get<0>(GetParam()); }
  base::TimeDelta duration() const { return std::get<1>(GetParam()); }
  base::TimeDelta fake_from_now() const { return std::get<2>(GetParam()); }
  bool is_off_hours_expected() const { return std::get<3>(GetParam()); }
};

TEST_P(OneOffHoursIntervalTest, CheckUpdateOffHoursPolicy) {
  device_settings_service_.SetDeviceOffHoursControllerForTesting(
      base::MakeUnique<policy::DeviceOffHoursController>(GetClock(),
                                                         GetTickClock()));
  system_clock_client()->set_network_synchronized(true);
  system_clock_client()->NotifyObserversSystemClockUpdated();
  policy::DeviceOffHoursController* device_off_hours_controller =
      device_settings_service_.device_off_hours_controller();

  em::ChromeDeviceSettingsProto& proto(device_policy_.payload());
  CreateAndSetOffHoursPolicyToProto(proto, start_in(), duration());
  UpdateDeviceSettings();

  SetFakeFromNow(fake_from_now());
  device_off_hours_controller->UpdateOffHoursPolicy(proto);
  EXPECT_EQ(device_off_hours_controller->is_off_hours_mode(),
            is_off_hours_expected());
}

// TODO: More cases
INSTANTIATE_TEST_CASE_P(
    TestCases,
    OneOffHoursIntervalTest,
    testing::Values(std::make_tuple(kHour, kHour, base::TimeDelta(), false),
                    std::make_tuple(kHour, kHour, kHour, true),
                    std::make_tuple(kHour, kHour, 1.5 * kHour, true),
                    std::make_tuple(kHour, kHour, 2 * kHour, false)));

class OffHoursWakeUpUpdate : public DeviceOffHoursControllerTest {
 public:
  OffHoursWakeUpUpdate() {
    std::unique_ptr<chromeos::DBusThreadManagerSetter> dbus_setter =
        chromeos::DBusThreadManager::GetSetterForTesting();
    power_manager_client_ = new chromeos::FakePowerManagerClient();
    dbus_setter->SetPowerManagerClient(base::WrapUnique(power_manager_client_));
  }

  chromeos::FakePowerManagerClient* power_manager() {
    return power_manager_client_;
  }

 private:
  chromeos::FakePowerManagerClient* power_manager_client_;

  DISALLOW_COPY_AND_ASSIGN(OffHoursWakeUpUpdate);
};

TEST_F(OffHoursWakeUpUpdate, CheckSendSuspendDone) {
  device_settings_service_.SetDeviceOffHoursControllerForTesting(
      base::MakeUnique<policy::DeviceOffHoursController>(GetClock(),
                                                         GetTickClock()));
  system_clock_client()->set_network_synchronized(true);
  system_clock_client()->NotifyObserversSystemClockUpdated();

  policy::DeviceOffHoursController* device_off_hours_controller =
      device_settings_service_.device_off_hours_controller();
  em::ChromeDeviceSettingsProto& proto(device_policy_.payload());
  CreateAndSetOffHoursPolicyToProto(proto, kDay, kHour);
  UpdateDeviceSettings();

  EXPECT_FALSE(device_off_hours_controller->is_off_hours_mode());

  SetFakeFromNow(kDay);
  power_manager()->SendSuspendDone();
  EXPECT_TRUE(device_off_hours_controller->is_off_hours_mode());

  SetFakeFromNow(kDay + kHour);
  power_manager()->SendSuspendDone();
  EXPECT_FALSE(device_off_hours_controller->is_off_hours_mode());
}

}  // namespace chromeos
