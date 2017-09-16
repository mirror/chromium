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

constexpr base::TimeDelta kMinute = base::TimeDelta::FromMinutes(1);
constexpr base::TimeDelta kHour = base::TimeDelta::FromHours(1);
constexpr base::TimeDelta kDay = base::TimeDelta::FromDays(1);

class FakeClock : public base::Clock, public base::TickClock {
 public:
  FakeClock() {
    fake_now_ = base::Time::Now();
    fake_now_ticks_ = base::TimeTicks::Now();
  }

  base::Time Now() override { return fake_now_; }

  base::TimeTicks NowTicks() override { return fake_now_ticks_; }

  void SetFakeTimeNow(base::Time now) { fake_now_ = now; }

  void SetFakeTimeTicksNow(base::TimeTicks now) { fake_now_ticks_ = now; }

  void SetFakeNow(base::TimeDelta duration) {
    SetFakeTimeNow(base::Time::Now() + duration);
    SetFakeTimeTicksNow(base::TimeTicks::Now() + duration);
  }

 private:
  base::Time fake_now_;
  base::TimeTicks fake_now_ticks_;
};

}  // namespace

class DeviceOffHoursControllerTest : public DeviceSettingsTestBase {
 protected:
  DeviceOffHoursControllerTest() {}

  void UpdateDeviceSettings() {
    device_policy_.Build();
    device_settings_test_helper_.set_policy_blob(device_policy_.GetBlob());
    ReloadDeviceSettings();
  }

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
    em::DeviceOffHoursIntervalProto interval =
        em::DeviceOffHoursIntervalProto();
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
  void CreateAndAddOffHoursPolicyToProto(
      base::TimeDelta delay = base::TimeDelta(),
      base::TimeDelta duration = kHour) {
    base::Time::Exploded exploded;
    base::Time::Now().UTCExplode(&exploded);

    int cur_day_of_week = exploded.day_of_week;
    if (cur_day_of_week == 0)
      cur_day_of_week = 7;
    int cur_time = exploded.hour * kHour.InMilliseconds() +
                   exploded.minute * kMinute.InMilliseconds() +
                   exploded.second * 1000;
    int start_time =
        (cur_time + delay.InMilliseconds()) % kDay.InMilliseconds();
    int day_offset =
        (cur_time + delay.InMilliseconds()) / kDay.InMilliseconds();
    int start_day_of_week = (cur_day_of_week + day_offset - 1) % 7 + 1;
    int end_time =
        (start_time + duration.InMilliseconds()) % kDay.InMilliseconds();
    day_offset =
        (start_time + duration.InMilliseconds()) / kDay.InMilliseconds();
    int end_day_of_week = (start_day_of_week + day_offset - 1) % 7 + 1;
    std::vector<em::DeviceOffHoursIntervalProto> intervals;
    intervals.push_back(CreateInterval(
        start_day_of_week, base::TimeDelta::FromMilliseconds(start_time),
        end_day_of_week, base::TimeDelta::FromMilliseconds(end_time)));
    AddOffHoursPolicyToProto(intervals, "GMT", {"guest_mode_enabled"});
  }

  std::unique_ptr<policy::DeviceOffHoursController>
      device_off_hours_controller_;
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
  // guest_mode_enabled = 1 [default = true]
  proto.mutable_guest_mode_enabled()->set_guest_mode_enabled(false);
  UpdateDeviceSettings();
  EXPECT_FALSE(device_settings_service_.device_settings()
                   ->guest_mode_enabled()
                   .guest_mode_enabled());
  CreateAndAddOffHoursPolicyToProto();
  UpdateDeviceSettings();
  EXPECT_TRUE(device_settings_service_.device_settings()
                  ->guest_mode_enabled()
                  .guest_mode_enabled());
}

TEST_F(DeviceOffHoursControllerTest, FakeClock) {
  FakeClock* clock = new FakeClock();
  device_off_hours_controller_.reset(
      new policy::DeviceOffHoursController(clock, clock));
  EXPECT_FALSE(device_off_hours_controller_->IsOffHoursMode());
  em::ChromeDeviceSettingsProto& proto(device_policy_.payload());
  CreateAndAddOffHoursPolicyToProto();
  device_off_hours_controller_->UpdateOffHoursMode(proto);
  EXPECT_TRUE(device_off_hours_controller_->IsOffHoursMode());
  clock->SetFakeNow(base::TimeDelta::FromHours(2));
  device_off_hours_controller_->UpdateOffHoursMode(proto);
  EXPECT_FALSE(device_off_hours_controller_->IsOffHoursMode());
}

TEST_F(DeviceOffHoursControllerTest, CheckOneDayInterval) {
  FakeClock* clock = new FakeClock();
  device_off_hours_controller_.reset(
      new policy::DeviceOffHoursController(clock, clock));
  em::ChromeDeviceSettingsProto& proto(device_policy_.payload());
  for (int start_hours = 1; start_hours < 24 * 7; start_hours++) {
    for (int duration_hours = 0; duration_hours < 24 * 7; duration_hours++) {
      base::TimeDelta start = base::TimeDelta::FromHours(start_hours);
      base::TimeDelta duration = base::TimeDelta::FromHours(duration_hours);
      //      LOG(ERROR) << start_hours << "; " << duration hours;
      clock->SetFakeNow(base::TimeDelta());
      RemoveOffHoursPolicyFromProto();
      CreateAndAddOffHoursPolicyToProto(start, duration);
      device_off_hours_controller_->UpdateOffHoursMode(proto);
      if (start_hours + duration_hours <= 168)
        EXPECT_FALSE(device_off_hours_controller_->IsOffHoursMode());
      else
        EXPECT_TRUE(device_off_hours_controller_->IsOffHoursMode());

      clock->SetFakeNow(start);
      device_off_hours_controller_->UpdateOffHoursMode(proto);

      if (duration_hours == 0) {
        EXPECT_FALSE(device_off_hours_controller_->IsOffHoursMode());
        continue;
      }

      EXPECT_TRUE(device_off_hours_controller_->IsOffHoursMode());

      clock->SetFakeNow(start + duration / 2);
      device_off_hours_controller_->UpdateOffHoursMode(proto);
      EXPECT_TRUE(device_off_hours_controller_->IsOffHoursMode());

      clock->SetFakeNow(start + duration);
      device_off_hours_controller_->UpdateOffHoursMode(proto);
      EXPECT_FALSE(device_off_hours_controller_->IsOffHoursMode());
    }
  }
}

}  // namespace chromeos
