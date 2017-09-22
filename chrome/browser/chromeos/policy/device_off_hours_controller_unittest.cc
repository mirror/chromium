// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/chromeos/policy/device_off_hours_controller.h"

#include <utility>

#include "base/time/default_clock.h"
#include "base/time/tick_clock.h"
#include "base/time/time.h"
#include "base/values.h"
#include "chrome/browser/chromeos/policy/proto/chrome_device_policy.pb.h"
#include "chrome/browser/chromeos/settings/device_settings_test_helper.h"

namespace em = enterprise_management;

namespace chromeos {

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

void AddOffHoursPolicyToProto(
    const std::vector<em::DeviceOffHoursIntervalProto>& intervals,
    const std::string& timezone,
    const std::vector<std::string>& ignored_policy) {
  em::ChromeDeviceSettingsProto& proto(device_policy_.payload());
  auto* off_hours = proto.mutable_device_off_hours();
  for (auto interval : intervals) {
    auto* cur = off_hours->add_intervals();
    *cur = interval;
  }
  off_hours->set_timezone(timezone);
  for (auto p : ignored_policy) {
    off_hours->add_ignored_policies(p);
  }
}

void RemoveOffHoursPolicyFromProto() {
  em::ChromeDeviceSettingsProto& proto(device_policy_.payload());
  proto.clear_device_off_hours();
}

em::DeviceOffHoursIntervalProto CreateInterval(int start_day_of_week,
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
void CreateAndAddOffHoursPolicyToProto(base::TimeDelta delay,
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
  intervals.push_back(
      CreateInterval(start_day_of_week, start_time, end_day_of_week, end_time));
  std::vector<std::string> ignored_policies = {"guest_mode_enabled"};
  AddOffHoursPolicyToProto(intervals, "GMT", ignored_policies);
}

}  // namespace

class DeviceOffHoursControllerTest : public testing::Test {
 protected:
  DeviceOffHoursControllerTest() {}

  void SetFakeNow(base::TimeDelta duration) {
    fake_clock_->SetFakeTimeNow(base::Time::Now() + duration);
    fake_tick_clock_->SetFakeTimeTicksNow(base::TimeTicks::Now() + duration);
  }

  base::Clock* GetClock() { return fake_clock_; }

  base::TickClock* GetTickClock() { return fake_tick_clock_; }

  std::unique_ptr<policy::DeviceOffHoursController>
      device_off_hours_controller_;

 private:
  FakeClock* fake_clock_ = new FakeClock();
  FakeTickClock* fake_tick_clock_ = new FakeTickClock();

  DISALLOW_COPY_AND_ASSIGN(DeviceOffHoursControllerTest);
};

class DeviceOffHoursControllerTest : public DeviceSettingsTestBase {
 protected:
  DeviceOffHoursControllerTest() {}

  void UpdateDeviceSettings() {
    device_policy_.Build();
    device_settings_test_helper_.set_policy_blob(device_policy_.GetBlob());
    ReloadDeviceSettings();
  }

 private:
  DISALLOW_COPY_AND_ASSIGN(DeviceOffHoursControllerTest);
};

TEST_F(DeviceOffHoursControllerTest, CheckOffHoursUnset) {
  enterprise_management::ChromeDeviceSettingsProto& proto(
      device_policy_.payload());
  proto.mutable_guest_mode_enabled()->set_guest_mode_enabled(false);
  UpdateDeviceSettings();
  EXPECT_FALSE(device_settings_service_.device_settings()
                   ->guest_mode_enabled()
                   .guest_mode_enabled());
  RemoveOffHoursPolicyFromProto();
  UpdateDeviceSettings();
  EXPECT_FALSE(device_settings_service_.device_settings()
                   ->guest_mode_enabled()
                   .guest_mode_enabled());
}

TEST_F(DeviceOffHoursControllerTest, CheckOffHoursModeOff) {
  enterprise_management::ChromeDeviceSettingsProto& proto(
      device_policy_.payload());
  proto.mutable_guest_mode_enabled()->set_guest_mode_enabled(false);
  UpdateDeviceSettings();
  EXPECT_FALSE(device_settings_service_.device_settings()
                   ->guest_mode_enabled()
                   .guest_mode_enabled());
  CreateAndAddOffHoursPolicyToProto(kHour, kHour);
  UpdateDeviceSettings();
  EXPECT_FALSE(device_settings_service_.device_settings()
                   ->guest_mode_enabled()
                   .guest_mode_enabled());
}

TEST_F(DeviceOffHoursControllerTest, CheckOffHoursModeOn) {
  enterprise_management::ChromeDeviceSettingsProto& proto(
      device_policy_.payload());
  proto.mutable_guest_mode_enabled()->set_guest_mode_enabled(false);
  UpdateDeviceSettings();
  EXPECT_FALSE(device_settings_service_.device_settings()
                   ->guest_mode_enabled()
                   .guest_mode_enabled());
  CreateAndAddOffHoursPolicyToProto(base::TimeDelta(), kHour);
  UpdateDeviceSettings();
  EXPECT_TRUE(device_settings_service_.device_settings()
                  ->guest_mode_enabled()
                  .guest_mode_enabled());
}

TEST_F(DeviceOffHoursControllerTest, GetTimezoneOffset) {
  //  FakeClock* clock = new FakeClock();
  // TODO(yakovleva): how tests anonymous namespace?
}

TEST_F(DeviceOffHoursControllerTest, ConvertOffHoursProtoToValue) {
  // TODO(yakovleva): write a test
}

TEST_F(DeviceOffHoursControllerTest, ApplyOffHoursPolicyToProto) {
  enterprise_management::ChromeDeviceSettingsProto& proto(
      device_policy_.payload());
  proto.mutable_guest_mode_enabled()->set_guest_mode_enabled(false);
  proto.mutable_allow_new_users()->set_allow_new_users(false);
  proto.mutable_camera_enabled()->set_camera_enabled(false);
  AddOffHoursPolicyToProto({}, "GMT",
                           {"guest_mode_enabled", "allow_new_users"});
  std::unique_ptr<em::ChromeDeviceSettingsProto> off_hours_proto =
      policy::ApplyOffHoursPolicyToProto(device_policy_.payload());
  EXPECT_TRUE(off_hours_proto->guest_mode_enabled().guest_mode_enabled());
  EXPECT_TRUE(off_hours_proto->allow_new_users().allow_new_users());
  EXPECT_FALSE(off_hours_proto->camera_enabled().camera_enabled());
  // TODO(yakovleva): add more examples?
}

TEST_F(DeviceOffHoursControllerTest, FakeClock) {
  device_off_hours_controller_.reset(
      new policy::DeviceOffHoursController(GetClock(), GetTickClock()));
  EXPECT_FALSE(device_off_hours_controller_->IsOffHoursMode());
  CreateAndAddOffHoursPolicyToProto(base::TimeDelta(), kHour);
  em::ChromeDeviceSettingsProto& proto(device_policy_.payload());
  device_off_hours_controller_->UpdateOffHoursPolicy(proto);
  EXPECT_TRUE(device_off_hours_controller_->IsOffHoursMode());
  SetFakeNow(base::TimeDelta::FromHours(2));
  device_off_hours_controller_->UpdateOffHoursPolicy(proto);
  EXPECT_FALSE(device_off_hours_controller_->IsOffHoursMode());
}

TEST_F(DeviceOffHoursControllerTest, CheckOneDayInterval) {
  device_off_hours_controller_.reset(
      new policy::DeviceOffHoursController(GetClock(), GetTickClock()));
  for (int start_hours = 1; start_hours < 24 * 7; start_hours++) {
    for (int duration_hours = 1; duration_hours < 24 * 7; duration_hours++) {
      base::TimeDelta start = base::TimeDelta::FromHours(start_hours);
      base::TimeDelta duration = base::TimeDelta::FromHours(duration_hours);
      //      LOG(ERROR) << start_hours << "; " << duration hours;
      SetFakeNow(base::TimeDelta());
      RemoveOffHoursPolicyFromProto();
      CreateAndAddOffHoursPolicyToProto(start, duration);
      em::ChromeDeviceSettingsProto& proto(device_policy_.payload());
      device_off_hours_controller_->UpdateOffHoursPolicy(proto);
      if (start_hours + duration_hours <= 168)
        EXPECT_FALSE(device_off_hours_controller_->IsOffHoursMode());
      else
        EXPECT_TRUE(device_off_hours_controller_->IsOffHoursMode());

      SetFakeNow(start);
      device_off_hours_controller_->UpdateOffHoursPolicy(proto);

      EXPECT_TRUE(device_off_hours_controller_->IsOffHoursMode());

      SetFakeNow(start + duration / 2);
      device_off_hours_controller_->UpdateOffHoursPolicy(proto);
      EXPECT_TRUE(device_off_hours_controller_->IsOffHoursMode());

      SetFakeNow(start + duration);
      device_off_hours_controller_->UpdateOffHoursPolicy(proto);
      EXPECT_FALSE(device_off_hours_controller_->IsOffHoursMode());
    }
  }
}

}  // namespace chromeos
