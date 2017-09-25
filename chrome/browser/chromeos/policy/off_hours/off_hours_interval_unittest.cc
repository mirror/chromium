// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/chromeos/policy/off_hours/off_hours_interval.h"

#include <utility>

#include "base/time/time.h"
#include "base/values.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace chromeos {

using namespace policy;
using namespace policy::off_hours;

namespace {

enum {
  kMonday = 1,
  kTuesday = 2,
  kWednesday = 3,
  kThursday = 4,
  kFriday = 5,
  kSaturday = 6,
  kSunday = 7,
};

const int kMinutesInHour = 60;

constexpr base::TimeDelta kMinute = base::TimeDelta::FromMinutes(1);

}  // namespace

class FourOffHoursIntervalTest
    : public testing::Test,
      public testing::WithParamInterface<std::tuple<int, int, int, int>> {
 protected:
  int start_day_of_week() const { return std::get<0>(GetParam()); }
  int start_time() const { return std::get<1>(GetParam()); }
  int end_day_of_week() const { return std::get<2>(GetParam()); }
  int end_time() const { return std::get<3>(GetParam()); }
};

TEST_P(FourOffHoursIntervalTest, Constructor) {
  WeeklyTime start =
      WeeklyTime(start_day_of_week(), start_time() * kMinute.InMilliseconds());
  WeeklyTime end =
      WeeklyTime(end_day_of_week(), end_time() * kMinute.InMilliseconds());
  OffHoursInterval interval = OffHoursInterval(start, end);
  EXPECT_EQ(interval.start().day_of_week(), start_day_of_week());
  EXPECT_EQ(interval.start().milliseconds(),
            start_time() * kMinute.InMilliseconds());
  EXPECT_EQ(interval.end().day_of_week(), end_day_of_week());
  EXPECT_EQ(interval.end().milliseconds(),
            end_time() * kMinute.InMilliseconds());
}

TEST_P(FourOffHoursIntervalTest, ToValue) {
  WeeklyTime start = WeeklyTime(start_day_of_week(), start_time());
  WeeklyTime end = WeeklyTime(end_day_of_week(), end_time());
  OffHoursInterval interval = OffHoursInterval(start, end);
  std::unique_ptr<base::DictionaryValue> interval_value = interval.ToValue();
  base::DictionaryValue* start_value;
  interval_value->GetDictionary("start", &start_value);
  base::DictionaryValue* end_value;
  interval_value->GetDictionary("end", &end_value);

  int day_of_week_value;
  int milliseconds_value;

  start_value->GetInteger("day_of_week", &day_of_week_value);
  start_value->GetInteger("time", &milliseconds_value);
  EXPECT_EQ(day_of_week_value, start_day_of_week());
  EXPECT_EQ(milliseconds_value, start_time());

  end_value->GetInteger("day_of_week", &day_of_week_value);
  end_value->GetInteger("time", &milliseconds_value);
  EXPECT_EQ(day_of_week_value, end_day_of_week());
  EXPECT_EQ(milliseconds_value, end_time());
}

INSTANTIATE_TEST_CASE_P(
    OneMinuteInterval,
    FourOffHoursIntervalTest,
    testing::Values(std::make_tuple(kWednesday,
                                    kMinutesInHour,
                                    kWednesday,
                                    kMinutesInHour + 1)));
INSTANTIATE_TEST_CASE_P(
    TheLongestInterval,
    FourOffHoursIntervalTest,
    testing::Values(
        std::make_tuple(kMonday, 0, kSunday, 24 * kMinutesInHour - 1)));

INSTANTIATE_TEST_CASE_P(
    RandomInterval,
    FourOffHoursIntervalTest,
    testing::Values(std::make_tuple(kTuesday,
                                    10 * kMinutesInHour,
                                    kFriday,
                                    14 * kMinutesInHour +
                                        15)));

class SixOffHoursIntervalTest
    : public testing::Test,
      public testing::WithParamInterface<
          std::tuple<int, int, int, int, int, int, bool>> {
 protected:
  int start_day_of_week() const { return std::get<0>(GetParam()); }
  int start_time() const { return std::get<1>(GetParam()); }
  int end_day_of_week() const { return std::get<2>(GetParam()); }
  int end_time() const { return std::get<3>(GetParam()); }
  int current_day_of_week() const { return std::get<4>(GetParam()); }
  int current_time() const { return std::get<5>(GetParam()); }
  bool expected_contains() const { return std::get<6>(GetParam()); }
};

TEST_P(SixOffHoursIntervalTest, Contains) {
  WeeklyTime start =
      WeeklyTime(start_day_of_week(), start_time() * kMinute.InMilliseconds());
  WeeklyTime end =
      WeeklyTime(end_day_of_week(), end_time() * kMinute.InMilliseconds());
  OffHoursInterval interval = OffHoursInterval(start, end);
  WeeklyTime weekly_time = WeeklyTime(
      current_day_of_week(), current_time() * kMinute.InMilliseconds());
  EXPECT_EQ(interval.Contains(weekly_time), expected_contains());
}

INSTANTIATE_TEST_CASE_P(
    InTheLongestInterval,
    SixOffHoursIntervalTest,
    testing::Values(std::make_tuple(kMonday,
                                    0,
                                    kSunday,
                                    24 * kMinutesInHour - 1,
                                    kWednesday,
                                    10 * kMinutesInHour,
                                    true)));

INSTANTIATE_TEST_CASE_P(
    OutTheLongestInterval,
    SixOffHoursIntervalTest,
    testing::Values(std::make_tuple(kSunday,
                                    24 * kMinutesInHour - 1,
                                    kMonday,
                                    0,
                                    kWednesday,
                                    10 * kMinutesInHour,
                                    false)));

INSTANTIATE_TEST_CASE_P(
    OutTheShortestInterval,
    SixOffHoursIntervalTest,
    testing::Values(std::make_tuple(kMonday,
                                    0,
                                    kMonday,
                                    1,
                                    kTuesday,
                                    9 * kMinutesInHour,
                                    false)));

INSTANTIATE_TEST_CASE_P(
    OutTheShortestInterval2,
    SixOffHoursIntervalTest,
    testing::Values(
        std::make_tuple(kMonday, 0, kMonday, 1, kMonday, 1, false)));

INSTANTIATE_TEST_CASE_P(
    InTheShortestInterval,
    SixOffHoursIntervalTest,
    testing::Values(std::make_tuple(kMonday, 0, kMonday, 1, kMonday, 0, true)));

INSTANTIATE_TEST_CASE_P(
    CheckStartInterval,
    SixOffHoursIntervalTest,
    testing::Values(std::make_tuple(kTuesday,
                                    10 * kMinutesInHour + 30,
                                    kFriday,
                                    14 * kMinutesInHour + 45,
                                    kTuesday,
                                    10 * kMinutesInHour + 30,
                                    true)));

INSTANTIATE_TEST_CASE_P(
    CheckEndInterval,
    SixOffHoursIntervalTest,
    testing::Values(std::make_tuple(kTuesday,
                                    10 * kMinutesInHour + 30,
                                    kFriday,
                                    14 * kMinutesInHour + 45,
                                    kFriday,
                                    14 * kMinutesInHour + 45,
                                    false)));

INSTANTIATE_TEST_CASE_P(
    InRandomInterval,
    SixOffHoursIntervalTest,
    testing::Values(std::make_tuple(kFriday,
                                    17 * kMinutesInHour +
                                        60,
                                    kMonday,
                                    9 * kMinutesInHour,
                                    kSunday,
                                    14 * kMinutesInHour,
                                    true)));

INSTANTIATE_TEST_CASE_P(
    OutRandomInterval,
    SixOffHoursIntervalTest,
    testing::Values(std::make_tuple(kMonday,
                                    9 * kMinutesInHour,
                                    kFriday,
                                    17 * kMinutesInHour,
                                    kSunday,
                                    14 * kMinutesInHour,
                                    false)));

}  // namespace chromeos
