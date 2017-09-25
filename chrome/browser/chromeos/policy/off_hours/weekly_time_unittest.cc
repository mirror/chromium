// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/chromeos/policy/off_hours/weekly_time.h"

#include <tuple>
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
constexpr base::TimeDelta kHour = base::TimeDelta::FromHours(1);
constexpr base::TimeDelta kWeek = base::TimeDelta::FromDays(7);

}  // namespace

class TwoWeeklyTimeTest
    : public testing::Test,
      public testing::WithParamInterface<std::tuple<int, int>> {
 public:
  int day_of_week() const { return std::get<0>(GetParam()); }
  int minutes() const { return std::get<1>(GetParam()); }
};

TEST_P(TwoWeeklyTimeTest, Constructor) {
  WeeklyTime weekly_time =
      WeeklyTime(day_of_week(), minutes() * kMinute.InMilliseconds());
  EXPECT_EQ(weekly_time.day_of_week(), day_of_week());
  EXPECT_EQ(weekly_time.milliseconds(), minutes() * kMinute.InMilliseconds());
}

TEST_P(TwoWeeklyTimeTest, ToValue) {
  WeeklyTime weekly_time =
      WeeklyTime(day_of_week(), minutes() * kMinute.InMilliseconds());
  std::unique_ptr<base::DictionaryValue> weekly_time_value =
      weekly_time.ToValue();
  int day_of_week_value;
  weekly_time_value->GetInteger("day_of_week", &day_of_week_value);
  int milliseconds_value;
  weekly_time_value->GetInteger("time", &milliseconds_value);
  EXPECT_EQ(day_of_week_value, day_of_week());
  EXPECT_EQ(milliseconds_value, minutes() * kMinute.InMilliseconds());
}

INSTANTIATE_TEST_CASE_P(
    TheSmallestCase,
    TwoWeeklyTimeTest,
    testing::Values(std::make_tuple(kMonday, 0)));

INSTANTIATE_TEST_CASE_P(
    TheBiggestCase,
    TwoWeeklyTimeTest,
    testing::Values(std::make_tuple(kSunday, 24 * kMinutesInHour - 1)));

INSTANTIATE_TEST_CASE_P(
    RandomCase,
    TwoWeeklyTimeTest,
    testing::Values(std::make_tuple(kWednesday, 15 * kMinutesInHour + 30)));

class FourWeeklyTimeTest
    : public testing::Test,
      public testing::WithParamInterface<
          std::tuple<int, int, int, int, base::TimeDelta>> {
 public:
  int day1() const { return std::get<0>(GetParam()); }
  int minutes1() const { return std::get<1>(GetParam()); }
  int day2() const { return std::get<2>(GetParam()); }
  int minutes2() const { return std::get<3>(GetParam()); }
  base::TimeDelta expected_duration() const { return std::get<4>(GetParam()); }
};

TEST_P(FourWeeklyTimeTest, GetDuration) {
  WeeklyTime weekly_time1 =
      WeeklyTime(day1(), minutes1() * kMinute.InMilliseconds());
  WeeklyTime weekly_time2 =
      WeeklyTime(day2(), minutes2() * kMinute.InMilliseconds());
  EXPECT_EQ(weekly_time1.GetDurationTo(weekly_time2), expected_duration());
}

INSTANTIATE_TEST_CASE_P(
    ZeroDuration,
    FourWeeklyTimeTest,
    testing::Values(std::make_tuple(kWednesday,
                                    kMinutesInHour,
                                    kWednesday,
                                    kMinutesInHour,
                                    base::TimeDelta())));

INSTANTIATE_TEST_CASE_P(
    HourDuration,
    FourWeeklyTimeTest,
    testing::Values(std::make_tuple(kThursday,
                                    54,
                                    kThursday,
                                    kMinutesInHour + 54,
                                    kHour)));

INSTANTIATE_TEST_CASE_P(
    TheLongestDuration,
    FourWeeklyTimeTest,
    testing::Values(std::make_tuple(kMonday,
                                    0,
                                    kSunday,
                                    24 * kMinutesInHour - 1,
                                    kWeek - kMinute)));

INSTANTIATE_TEST_CASE_P(
    MinuteDuration,
    FourWeeklyTimeTest,
    testing::Values(std::make_tuple(kSunday,
                                    24 * kMinutesInHour - 1,
                                    kMonday,
                                    0,
                                    kMinute)));

INSTANTIATE_TEST_CASE_P(
    RandomDuration,
    FourWeeklyTimeTest,
    testing::Values(std::make_tuple(kSaturday,
                                    15 * kMinutesInHour + 30,
                                    kFriday,
                                    17 * kMinutesInHour + 45,
                                    base::TimeDelta::FromMinutes(8775))));

class ThreeWeeklyTimeTest
    : public testing::Test,
      public testing::WithParamInterface<std::tuple<int, int, int, int, int>> {
 public:
  int day_of_week() const { return std::get<0>(GetParam()); }
  int minutes() const { return std::get<1>(GetParam()); }
  int gmt_offset_minutes() const { return std::get<2>(GetParam()); }
  int expected_day() const { return std::get<3>(GetParam()); }
  int expected_minutes() const { return std::get<4>(GetParam()); }
};

TEST_P(ThreeWeeklyTimeTest, ConvertToGmt) {
  WeeklyTime weekly_time =
      WeeklyTime(day_of_week(), minutes() * kMinute.InMilliseconds());
  WeeklyTime weekly_time_utc =
      weekly_time.ConvertToGmt(gmt_offset_minutes() * kMinute.InMilliseconds());

  EXPECT_EQ(weekly_time_utc.day_of_week(), expected_day());
  EXPECT_EQ(weekly_time_utc.milliseconds(),
            expected_minutes() * kMinute.InMilliseconds());
}

INSTANTIATE_TEST_CASE_P(
    ZeroOffset,
    ThreeWeeklyTimeTest,
    testing::Values(std::make_tuple(kTuesday,
                                    15 * kMinutesInHour + 30,
                                    0,
                                    kTuesday,
                                    15 * kMinutesInHour + 30)));

INSTANTIATE_TEST_CASE_P(
    TheSmallestOffset,
    ThreeWeeklyTimeTest,
    testing::Values(std::make_tuple(kTuesday,
                                    10 * kMinutesInHour + 30,
                                    -13 * kMinutesInHour,
                                    kTuesday,
                                    23 * kMinutesInHour + 30)));

INSTANTIATE_TEST_CASE_P(
    TheSmallestOffsetToNextDay,
    ThreeWeeklyTimeTest,
    testing::Values(std::make_tuple(kSunday,
                                    21 * kMinutesInHour + 30,
                                    -13 * kMinutesInHour,
                                    kMonday,
                                    10 * kMinutesInHour + 30)));

INSTANTIATE_TEST_CASE_P(
    TheBiggestOffset,
    ThreeWeeklyTimeTest,
    testing::Values(std::make_tuple(kWednesday,
                                    15 * kMinutesInHour + 30,
                                    13 * kMinutesInHour,
                                    kWednesday,
                                    2 * kMinutesInHour + 30)));

INSTANTIATE_TEST_CASE_P(
    TheBiggestOffsetToPreviousDay,
    ThreeWeeklyTimeTest,
    testing::Values(std::make_tuple(kMonday,
                                    9 * kMinutesInHour + 30,
                                    13 * kMinutesInHour,
                                    kSunday,
                                    20 * kMinutesInHour + 30)));

INSTANTIATE_TEST_CASE_P(
    HalfAnHourOffset,
    ThreeWeeklyTimeTest,
    testing::Values(std::make_tuple(kWednesday,
                                    10 * kMinutesInHour + 47,
                                    5 * kMinutesInHour + 30,
                                    kWednesday,
                                    5 * kMinutesInHour + 17)));

INSTANTIATE_TEST_CASE_P(
    FifteenMinutesOffset,
    ThreeWeeklyTimeTest,
    testing::Values(std::make_tuple(kMonday,
                                    10 * kMinutesInHour + 47,
                                    6 * kMinutesInHour + 15,
                                    kMonday,
                                    4 * kMinutesInHour + 32)));

INSTANTIATE_TEST_CASE_P(
    RandomOffset,
    ThreeWeeklyTimeTest,
    testing::Values(std::make_tuple(kThursday,
                                    22 * kMinutesInHour + 24,
                                    7 * kMinutesInHour,
                                    kThursday,
                                    15 * kMinutesInHour + 24)));

}  // namespace chromeos
