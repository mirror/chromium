// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/chromeos/policy/off_hours/device_off_hours_controller.h"

#include <utility>

#include "base/memory/ptr_util.h"
#include "base/test/simple_test_clock.h"
#include "base/test/simple_test_tick_clock.h"
#include "base/time/default_clock.h"
#include "base/time/tick_clock.h"
#include "base/time/time.h"
#include "base/values.h"
#include "chrome/browser/chromeos/policy/proto/chrome_device_policy.pb.h"
#include "chrome/browser/chromeos/settings/device_settings_test_helper.h"
#include "chromeos/dbus/dbus_thread_manager.h"
#include "chromeos/dbus/fake_power_manager_client.h"
#include "chromeos/dbus/fake_system_clock_client.h"

namespace em = enterprise_management;

namespace chromeos {

using policy::off_hours::OffHoursInterval;
using policy::off_hours::WeeklyTime;

namespace {

enum kWeekday {
  DAY_OF_WEEK_UNSPECIFIED = 0,
  MONDAY,
  TUESDAY,
  WEDNESDAY,
  THURSDAY,
  FRIDAY,
  SATURDAY,
  SUNDAY,
};

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

const std::string UTC_TIMEZONE = "UTC";

const int kDeviceAllowNewUsersTag = 3;
const int kDeviceGuestModeEnabledTag = 8;

struct OffHoursPolicy {
  std::string timezone;
  std::vector<OffHoursInterval> intervals;
  std::vector<int> ignored_policy_proto_tags;

  OffHoursPolicy(const std::string& timezone,
                 const std::vector<OffHoursInterval>& intervals,
                 const std::vector<int>& ignored_policy_proto_tags)
      : timezone(timezone),
        intervals(intervals),
        ignored_policy_proto_tags(ignored_policy_proto_tags) {}
};

em::DeviceOffHoursIntervalProto ConvertOffHoursIntervalToProto(
    const OffHoursInterval& off_hours_interval) {
  em::DeviceOffHoursIntervalProto interval_proto;
  em::WeeklyTimeProto* start = interval_proto.mutable_start();
  em::WeeklyTimeProto* end = interval_proto.mutable_end();
  start->set_day_of_week(kWeekdays[off_hours_interval.start().day_of_week()]);
  start->set_time(off_hours_interval.start().milliseconds());
  end->set_day_of_week(kWeekdays[off_hours_interval.end().day_of_week()]);
  end->set_time(off_hours_interval.end().milliseconds());
  return interval_proto;
}

void RemoveOffHoursPolicyFromProto(em::ChromeDeviceSettingsProto* proto) {
  proto->clear_device_off_hours();
}

void SetOffHoursPolicyToProto(em::ChromeDeviceSettingsProto* proto,
                              const OffHoursPolicy& off_hours_policy) {
  RemoveOffHoursPolicyFromProto(proto);
  auto* off_hours = proto->mutable_device_off_hours();
  for (auto interval : off_hours_policy.intervals) {
    auto interval_proto = ConvertOffHoursIntervalToProto(interval);
    auto* cur = off_hours->add_intervals();
    *cur = interval_proto;
  }
  off_hours->set_timezone(off_hours_policy.timezone);
  for (auto p : off_hours_policy.ignored_policy_proto_tags) {
    off_hours->add_ignored_policy_proto_tags(p);
  }
}

}  // namespace

class DeviceOffHoursControllerSimpleTest : public DeviceSettingsTestBase {
 protected:
  DeviceOffHoursControllerSimpleTest() {}

  void SetUp() override {
    DeviceSettingsTestBase::SetUp();
    system_clock_client_ = new chromeos::FakeSystemClockClient();
    dbus_setter_->SetSystemClockClient(base::WrapUnique(system_clock_client_));
    power_manager_client_ = new chromeos::FakePowerManagerClient();
    dbus_setter_->SetPowerManagerClient(
        base::WrapUnique(power_manager_client_));

    device_settings_service_.SetDeviceOffHoursControllerForTesting(
        base::MakeUnique<policy::off_hours::DeviceOffHoursController>());
    device_off_hours_controller_ =
        device_settings_service_.device_off_hours_controller();
  }

  void UpdateDeviceSettings() {
    device_policy_.Build();
    device_settings_test_helper_.set_policy_blob(device_policy_.GetBlob());
    ReloadDeviceSettings();
  }

  // Return number of weekday from 1 to 7 in |input_time|. (1 = Monday etc.)
  int ExtractDayOfWeek(base::Time input_time) {
    base::Time::Exploded exploded;
    input_time.UTCExplode(&exploded);
    int current_day_of_week = exploded.day_of_week;
    if (current_day_of_week == 0)
      current_day_of_week = 7;
    return current_day_of_week;
  }

  // Return next day of week. |day_of_week| and return value are from 1 to 7. (1
  // = Monday etc.)
  int NextDayOfWeek(int day_of_week) { return day_of_week % 7 + 1; }

  chromeos::FakeSystemClockClient* system_clock_client() {
    return system_clock_client_;
  }

  chromeos::FakePowerManagerClient* power_manager() {
    return power_manager_client_;
  }

  policy::DeviceOffHoursController* device_off_hours_controller() {
    return device_off_hours_controller_;
  }

 private:
  // The object is owned by DeviceSettingsTestBase class.
  chromeos::FakeSystemClockClient* system_clock_client_;

  // The object is owned by DeviceSettingsTestBase class.
  chromeos::FakePowerManagerClient* power_manager_client_;

  // The object is owned by DeviceSettingsService class.
  policy::off_hours::DeviceOffHoursController* device_off_hours_controller_;

  DISALLOW_COPY_AND_ASSIGN(DeviceOffHoursControllerSimpleTest);
};

TEST_F(DeviceOffHoursControllerSimpleTest, CheckOffHoursUnset) {
  system_clock_client()->set_network_synchronized(true);
  system_clock_client()->NotifyObserversSystemClockUpdated();
  em::ChromeDeviceSettingsProto& proto(device_policy_.payload());
  proto.mutable_guest_mode_enabled()->set_guest_mode_enabled(false);
  UpdateDeviceSettings();
  EXPECT_FALSE(device_settings_service_.device_settings()
                   ->guest_mode_enabled()
                   .guest_mode_enabled());
  RemoveOffHoursPolicyFromProto(&proto);
  UpdateDeviceSettings();
  EXPECT_FALSE(device_settings_service_.device_settings()
                   ->guest_mode_enabled()
                   .guest_mode_enabled());
}

TEST_F(DeviceOffHoursControllerSimpleTest, CheckOffHoursModeOff) {
  system_clock_client()->set_network_synchronized(true);
  system_clock_client()->NotifyObserversSystemClockUpdated();
  em::ChromeDeviceSettingsProto& proto(device_policy_.payload());
  proto.mutable_guest_mode_enabled()->set_guest_mode_enabled(false);
  UpdateDeviceSettings();
  EXPECT_FALSE(device_settings_service_.device_settings()
                   ->guest_mode_enabled()
                   .guest_mode_enabled());
  base::Time current_time = base::Time::Now();
  SetOffHoursPolicyToProto(
      &proto,
      OffHoursPolicy(
          UTC_TIMEZONE,
          {OffHoursInterval(
              WeeklyTime(NextDayOfWeek(ExtractDayOfWeek(current_time)),
                         base::TimeDelta::FromHours(10).InMilliseconds()),
              WeeklyTime(NextDayOfWeek(ExtractDayOfWeek(current_time)),
                         base::TimeDelta::FromHours(15).InMilliseconds()))},
          {kDeviceAllowNewUsersTag, kDeviceGuestModeEnabledTag}));
  UpdateDeviceSettings();
  EXPECT_FALSE(device_settings_service_.device_settings()
                   ->guest_mode_enabled()
                   .guest_mode_enabled());
}

TEST_F(DeviceOffHoursControllerSimpleTest, CheckOffHoursModeOn) {
  system_clock_client()->set_network_synchronized(true);
  system_clock_client()->NotifyObserversSystemClockUpdated();
  em::ChromeDeviceSettingsProto& proto(device_policy_.payload());
  proto.mutable_guest_mode_enabled()->set_guest_mode_enabled(false);
  UpdateDeviceSettings();
  EXPECT_FALSE(device_settings_service_.device_settings()
                   ->guest_mode_enabled()
                   .guest_mode_enabled());
  base::Time current_time = base::Time::Now();
  SetOffHoursPolicyToProto(
      &proto,
      OffHoursPolicy(
          UTC_TIMEZONE,
          {OffHoursInterval(
              WeeklyTime(ExtractDayOfWeek(current_time), 0),
              WeeklyTime(NextDayOfWeek(ExtractDayOfWeek(current_time)),
                         base::TimeDelta::FromHours(10).InMilliseconds()))},
          {kDeviceAllowNewUsersTag, kDeviceGuestModeEnabledTag}));
  UpdateDeviceSettings();
  EXPECT_TRUE(device_settings_service_.device_settings()
                  ->guest_mode_enabled()
                  .guest_mode_enabled());
}

TEST_F(DeviceOffHoursControllerSimpleTest, NoNetworkSynchronization) {
  system_clock_client()->set_network_synchronized(false);
  system_clock_client()->NotifyObserversSystemClockUpdated();
  em::ChromeDeviceSettingsProto& proto(device_policy_.payload());
  proto.mutable_guest_mode_enabled()->set_guest_mode_enabled(false);
  UpdateDeviceSettings();
  EXPECT_FALSE(device_settings_service_.device_settings()
                   ->guest_mode_enabled()
                   .guest_mode_enabled());
  base::Time current_time = base::Time::Now();
  SetOffHoursPolicyToProto(
      &proto,
      OffHoursPolicy(
          UTC_TIMEZONE,
          {OffHoursInterval(
              WeeklyTime(ExtractDayOfWeek(current_time), 0),
              WeeklyTime(NextDayOfWeek(ExtractDayOfWeek(current_time)),
                         base::TimeDelta::FromHours(10).InMilliseconds()))},
          {kDeviceAllowNewUsersTag, kDeviceGuestModeEnabledTag}));
  EXPECT_FALSE(device_settings_service_.device_settings()
                   ->guest_mode_enabled()
                   .guest_mode_enabled());
}

// -------------------------------

TEST_F(DeviceOffHoursControllerSimpleTest, GetTimezoneOffset) {
  //  FakeClock* clock = new FakeClock();
  // TODO(yakovleva): how tests anonymous namespace?
}

TEST_F(DeviceOffHoursControllerSimpleTest, ConvertOffHoursProtoToValue) {
  WeeklyTime start = WeeklyTime(1, kHour.InMilliseconds());
  WeeklyTime end = WeeklyTime(3, kHour.InMilliseconds() * 2);
  std::vector<OffHoursInterval> intervals = {OffHoursInterval(start, end)};
  std::vector<int> ignored_policy_tags = {kDeviceAllowNewUsersTag,
                                          kDeviceGuestModeEnabledTag};
  em::ChromeDeviceSettingsProto proto;
  SetOffHoursPolicyToProto(
      &proto, OffHoursPolicy(UTC_TIMEZONE, intervals, ignored_policy_tags));

  std::unique_ptr<base::DictionaryValue> off_hours_value =
      policy::ConvertOffHoursProtoToValue(proto.device_off_hours());

  base::DictionaryValue off_hours_expected;
  off_hours_expected.SetString("timezone", UTC_TIMEZONE);
  auto intervals_value = base::MakeUnique<base::ListValue>();
  for (const auto& interval : intervals)
    intervals_value->Append(interval.ToValue());
  off_hours_expected.SetList("intervals", std::move(intervals_value));
  auto ignored_policies_value = base::MakeUnique<base::ListValue>();
  for (const auto& policy : ignored_policy_tags)
    ignored_policies_value->GetList().emplace_back(policy);
  off_hours_expected.SetList("ignored_policy_proto_tags",
                             std::move(ignored_policies_value));

  EXPECT_EQ(*off_hours_value, off_hours_expected);
}

TEST_F(DeviceOffHoursControllerSimpleTest, ApplyOffHoursPolicyToProto) {
  em::ChromeDeviceSettingsProto proto;
  proto.mutable_guest_mode_enabled()->set_guest_mode_enabled(false);
  proto.mutable_allow_new_users()->set_allow_new_users(false);
  proto.mutable_camera_enabled()->set_camera_enabled(false);
  SetOffHoursPolicyToProto(
      &proto,
      OffHoursPolicy(UTC_TIMEZONE, {},
                     {kDeviceAllowNewUsersTag, kDeviceGuestModeEnabledTag}));
  std::unique_ptr<em::ChromeDeviceSettingsProto> off_hours_proto =
      policy::ApplyOffHoursPolicyToProto(proto);
  EXPECT_TRUE(off_hours_proto->guest_mode_enabled().guest_mode_enabled());
  EXPECT_TRUE(off_hours_proto->allow_new_users().allow_new_users());
  EXPECT_FALSE(off_hours_proto->camera_enabled().camera_enabled());
}

// --------------------------------------

class DeviceOffHoursControllerFakeClockTest
    : public DeviceOffHoursControllerSimpleTest {
 protected:
  DeviceOffHoursControllerFakeClockTest() {}

  void SetUp() override {
    DeviceOffHoursControllerSimpleTest::SetUp();
    system_clock_client()->set_network_synchronized(true);
    system_clock_client()->NotifyObserversSystemClockUpdated();
    std::unique_ptr<base::SimpleTestClock> test_clock =
        base::MakeUnique<base::SimpleTestClock>();
    test_clock_ = test_clock.get();
    // Clocks are set to 1970-01-01 00:00:00 UTC, Thursday.
    test_tick_clock_ = base::MakeUnique<base::SimpleTestTickClock>();
    test_clock_->SetNow(base::Time::UnixEpoch());
    test_tick_clock_->SetNowTicks(base::TimeTicks::UnixEpoch());
    device_off_hours_controller()->SetClockForTesting(std::move(test_clock),
                                                      test_tick_clock_.get());
  }

  void AdvanceTestClock(base::TimeDelta duration) {
    test_clock_->Advance(duration);
    test_tick_clock_->Advance(duration);
  }

  base::Clock* clock() { return test_clock_; }

  base::TickClock* tick_clock() { return test_tick_clock_.get(); }

 private:
  // The object is owned by DeviceOffHoursController class.
  base::SimpleTestClock* test_clock_;

  std::unique_ptr<base::SimpleTestTickClock> test_tick_clock_;

  DISALLOW_COPY_AND_ASSIGN(DeviceOffHoursControllerFakeClockTest);
};

TEST_F(DeviceOffHoursControllerFakeClockTest, FakeClock) {
  EXPECT_FALSE(device_off_hours_controller()->is_off_hours_mode());
  base::Time current_time = clock()->Now();
  em::ChromeDeviceSettingsProto& proto(device_policy_.payload());
  SetOffHoursPolicyToProto(
      &proto,
      OffHoursPolicy(
          UTC_TIMEZONE,
          {OffHoursInterval(
              WeeklyTime(ExtractDayOfWeek(current_time),
                         base::TimeDelta::FromHours(14).InMilliseconds()),
              WeeklyTime(ExtractDayOfWeek(current_time),
                         base::TimeDelta::FromHours(15).InMilliseconds()))},
          {kDeviceAllowNewUsersTag, kDeviceGuestModeEnabledTag}));
  AdvanceTestClock(base::TimeDelta::FromHours(14));
  UpdateDeviceSettings();
  EXPECT_TRUE(device_off_hours_controller()->is_off_hours_mode());
  AdvanceTestClock(base::TimeDelta::FromHours(1));
  UpdateDeviceSettings();
  EXPECT_FALSE(device_off_hours_controller()->is_off_hours_mode());
}

TEST_F(DeviceOffHoursControllerFakeClockTest, CheckSendSuspendDone) {
  int current_day_of_week = ExtractDayOfWeek(clock()->Now());
  LOG(ERROR) << "day " << current_day_of_week;
  em::ChromeDeviceSettingsProto& proto(device_policy_.payload());
  SetOffHoursPolicyToProto(
      &proto,
      OffHoursPolicy(
          UTC_TIMEZONE,
          {OffHoursInterval(WeeklyTime(NextDayOfWeek(current_day_of_week), 0),
                            WeeklyTime(NextDayOfWeek(current_day_of_week),
                                       kHour.InMilliseconds()))},
          {kDeviceAllowNewUsersTag, kDeviceGuestModeEnabledTag}));
  UpdateDeviceSettings();
  EXPECT_FALSE(device_off_hours_controller()->is_off_hours_mode());

  AdvanceTestClock(kDay);
  power_manager()->SendSuspendDone();
  EXPECT_TRUE(device_off_hours_controller()->is_off_hours_mode());

  AdvanceTestClock(kHour);
  power_manager()->SendSuspendDone();
  EXPECT_FALSE(device_off_hours_controller()->is_off_hours_mode());
}

class OneOffHoursIntervalTest
    : public DeviceOffHoursControllerFakeClockTest,
      public testing::WithParamInterface<
          std::tuple<OffHoursPolicy, base::TimeDelta, bool>> {
 public:
  OffHoursPolicy off_hours_policy() const { return std::get<0>(GetParam()); }
  base::TimeDelta advance_clock() const { return std::get<1>(GetParam()); }
  bool is_off_hours_expected() const { return std::get<2>(GetParam()); }
};

TEST_P(OneOffHoursIntervalTest, CheckUpdateOffHoursPolicy) {
  em::ChromeDeviceSettingsProto& proto(device_policy_.payload());
  SetOffHoursPolicyToProto(&proto, off_hours_policy());
  AdvanceTestClock(advance_clock());
  UpdateDeviceSettings();
  EXPECT_EQ(device_off_hours_controller()->is_off_hours_mode(),
            is_off_hours_expected());
}

INSTANTIATE_TEST_CASE_P(
    TestCases,
    OneOffHoursIntervalTest,
    testing::Values(
        std::make_tuple(
            OffHoursPolicy(
                UTC_TIMEZONE,
                {OffHoursInterval(
                    WeeklyTime(THURSDAY,
                               base::TimeDelta::FromHours(1).InMilliseconds()),
                    WeeklyTime(
                        THURSDAY,
                        base::TimeDelta::FromHours(2).InMilliseconds()))},
                {kDeviceAllowNewUsersTag, kDeviceGuestModeEnabledTag}),
            kHour,
            true),
        std::make_tuple(
            OffHoursPolicy(
                UTC_TIMEZONE,
                {OffHoursInterval(
                    WeeklyTime(THURSDAY,
                               base::TimeDelta::FromHours(1).InMilliseconds()),
                    WeeklyTime(
                        THURSDAY,
                        base::TimeDelta::FromHours(2).InMilliseconds()))},
                {kDeviceAllowNewUsersTag, kDeviceGuestModeEnabledTag}),
            kHour * 2,
            false),
        std::make_tuple(
            OffHoursPolicy(
                UTC_TIMEZONE,
                {OffHoursInterval(
                    WeeklyTime(THURSDAY,
                               base::TimeDelta::FromHours(1).InMilliseconds()),
                    WeeklyTime(
                        THURSDAY,
                        base::TimeDelta::FromHours(2).InMilliseconds()))},
                {kDeviceAllowNewUsersTag, kDeviceGuestModeEnabledTag}),
            kHour * 1.5,
            true),
        std::make_tuple(
            OffHoursPolicy(
                UTC_TIMEZONE,
                {OffHoursInterval(
                    WeeklyTime(THURSDAY,
                               base::TimeDelta::FromHours(1).InMilliseconds()),
                    WeeklyTime(
                        THURSDAY,
                        base::TimeDelta::FromHours(2).InMilliseconds()))},
                {kDeviceAllowNewUsersTag, kDeviceGuestModeEnabledTag}),
            kHour * 3,
            false)));

}  // namespace chromeos
