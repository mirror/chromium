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

constexpr base::TimeDelta kMinute = base::TimeDelta::FromMinutes(1);

}  // namespace

class OffHoursIntervalTest : public testing::Test {
 protected:
  OffHoursIntervalTest() {}
};

class OffHoursIntervalConstructorAndToValue
    : public testing::Test,
      public testing::WithParamInterface<std::tuple<int, int, int, int>> {};

TEST_P(OffHoursIntervalConstructorAndToValue, Constructor) {
  int day1;
  int minutes1;
  int day2;
  int minutes2;
  std::tie(day1, minutes1, day2, minutes2) = GetParam();
  WeeklyTime weekly_time1 =
      WeeklyTime(day1, minutes1 * kMinute.InMilliseconds());
  WeeklyTime weekly_time2 =
      WeeklyTime(day2, minutes2 * kMinute.InMilliseconds());
  OffHoursInterval interval1 = OffHoursInterval(weekly_time1, weekly_time2);
  EXPECT_EQ(interval1.start().day_of_week(), day1);
  EXPECT_EQ(interval1.start().milliseconds(),
            minutes1 * kMinute.InMilliseconds());
  EXPECT_EQ(interval1.end().day_of_week(), day2);
  EXPECT_EQ(interval1.end().milliseconds(),
            minutes2 * kMinute.InMilliseconds());
  OffHoursInterval interval2 = OffHoursInterval(weekly_time2, weekly_time1);
  EXPECT_EQ(interval2.start().day_of_week(), day2);
  EXPECT_EQ(interval2.start().milliseconds(),
            minutes2 * kMinute.InMilliseconds());
  EXPECT_EQ(interval2.end().day_of_week(), day1);
  EXPECT_EQ(interval2.end().milliseconds(),
            minutes1 * kMinute.InMilliseconds());
}

TEST_P(OffHoursIntervalConstructorAndToValue, ToValue) {
  int start_day_of_week;
  int start_time;
  int end_day_of_week;
  int end_time;
  std::tie(start_day_of_week, start_time, end_day_of_week, end_time) =
      GetParam();
  WeeklyTime start = WeeklyTime(start_day_of_week, start_time);
  WeeklyTime end = WeeklyTime(end_day_of_week, end_time);
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
  EXPECT_EQ(day_of_week_value, start_day_of_week);
  EXPECT_EQ(milliseconds_value, start_time);

  end_value->GetInteger("day_of_week", &day_of_week_value);
  end_value->GetInteger("time", &milliseconds_value);
  EXPECT_EQ(day_of_week_value, end_day_of_week);
  EXPECT_EQ(milliseconds_value, end_time);
}

INSTANTIATE_TEST_CASE_P(OneMinuteInterval,
                        OffHoursIntervalConstructorAndToValue,
                        testing::Combine(testing::Values(3),
                                         testing::Values(60),
                                         testing::Values(3),
                                         testing::Values(60 + 1)));

INSTANTIATE_TEST_CASE_P(TheLongestInterval,
                        OffHoursIntervalConstructorAndToValue,
                        testing::Combine(testing::Values(1),
                                         testing::Values(0),
                                         testing::Values(7),
                                         testing::Values(24 * 60 - 1)));

INSTANTIATE_TEST_CASE_P(RandomInterval,
                        OffHoursIntervalConstructorAndToValue,
                        testing::Combine(testing::Values(2),
                                         testing::Values(10 * 60),
                                         testing::Values(5),
                                         testing::Values(14 * 60 + 15)));

class OffHoursIntervalContains : public testing::Test,
                                 public testing::WithParamInterface<
                                     std::tuple<int, int, int, int, int, int>> {
};

TEST_P(OffHoursIntervalContains, Contains) {
  int day1;
  int minutes1;
  int day2;
  int minutes2;
  int day3;
  int minutes3;
  std::tie(day1, minutes1, day2, minutes2, day3, minutes3) = GetParam();
  WeeklyTime start = WeeklyTime(day1, minutes1 * kMinute.InMilliseconds());
  WeeklyTime end = WeeklyTime(day2, minutes2 * kMinute.InMilliseconds());
  OffHoursInterval interval = OffHoursInterval(start, end);
  WeeklyTime weekly_time =
      WeeklyTime(day3, minutes3 * kMinute.InMilliseconds());

  base::TimeDelta interval_duration = start.GetDurationTo(end);
  if (weekly_time.GetDurationTo(end).is_zero() ||
      (start.GetDurationTo(weekly_time) + weekly_time.GetDurationTo(end) !=
       interval_duration))
    EXPECT_FALSE(interval.Contains(weekly_time));
  else
    EXPECT_TRUE(interval.Contains(weekly_time));
}

INSTANTIATE_TEST_CASE_P(InTheLongestInterval,
                        OffHoursIntervalContains,
                        testing::Combine(testing::Values(1),
                                         testing::Values(0),
                                         testing::Values(7),
                                         testing::Values(24 * 60 - 1),
                                         testing::Values(3),
                                         testing::Values(10 * 60)));

INSTANTIATE_TEST_CASE_P(OutTheLongestInterval,
                        OffHoursIntervalContains,
                        testing::Combine(testing::Values(7),
                                         testing::Values(24 * 60 - 1),
                                         testing::Values(1),
                                         testing::Values(0),
                                         testing::Values(3),
                                         testing::Values(10 * 60)));

INSTANTIATE_TEST_CASE_P(OutTheShortestInterval,
                        OffHoursIntervalContains,
                        testing::Combine(testing::Values(1),
                                         testing::Values(0),
                                         testing::Values(1),
                                         testing::Values(1),
                                         testing::Values(2),
                                         testing::Values(9 * 60)));

INSTANTIATE_TEST_CASE_P(OutTheShortestInterval2,
                        OffHoursIntervalContains,
                        testing::Combine(testing::Values(1),
                                         testing::Values(0),
                                         testing::Values(1),
                                         testing::Values(1),
                                         testing::Values(1),
                                         testing::Values(1)));

INSTANTIATE_TEST_CASE_P(InTheShortestInterval,
                        OffHoursIntervalContains,
                        testing::Combine(testing::Values(1),
                                         testing::Values(0),
                                         testing::Values(1),
                                         testing::Values(1),
                                         testing::Values(1),
                                         testing::Values(0)));

INSTANTIATE_TEST_CASE_P(CheckStartInterval,
                        OffHoursIntervalContains,
                        testing::Combine(testing::Values(2),
                                         testing::Values(10 * 60 + 30),
                                         testing::Values(5),
                                         testing::Values(14 * 60 + 45),
                                         testing::Values(2),
                                         testing::Values(10 * 60 + 30)));

INSTANTIATE_TEST_CASE_P(CheckEndInterval,
                        OffHoursIntervalContains,
                        testing::Combine(testing::Values(2),
                                         testing::Values(10 * 60 + 30),
                                         testing::Values(5),
                                         testing::Values(14 * 60 + 45),
                                         testing::Values(5),
                                         testing::Values(14 * 60 + 45)));

INSTANTIATE_TEST_CASE_P(InRandomInterval,
                        OffHoursIntervalContains,
                        testing::Combine(testing::Values(5),
                                         testing::Values(17 * 60),
                                         testing::Values(1),
                                         testing::Values(9 * 60),
                                         testing::Values(7),
                                         testing::Values(14 * 60)));

INSTANTIATE_TEST_CASE_P(OutRandomInterval,
                        OffHoursIntervalContains,
                        testing::Combine(testing::Values(1),
                                         testing::Values(9 * 60),
                                         testing::Values(5),
                                         testing::Values(17 * 60),
                                         testing::Values(7),
                                         testing::Values(14 * 60)));

}  // namespace chromeos
