// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/chromeos/policy/off_hours/off_hours_weekly_time.h"

#include <tuple>
#include <utility>

#include "base/time/time.h"
#include "base/values.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace chromeos {

using namespace policy::weekly_time;

namespace {

constexpr base::TimeDelta kMinute = base::TimeDelta::FromMinutes(1);
constexpr base::TimeDelta kDay = base::TimeDelta::FromDays(1);
constexpr base::TimeDelta kWeek = base::TimeDelta::FromDays(7);

}  // namespace

class WeeklyTimeConstructorAndToValue
    : public testing::Test,
      public testing::WithParamInterface<std::tuple<int, int>> {};

TEST_P(WeeklyTimeConstructorAndToValue, Constructor) {
  int day_of_week;
  int minutes;
  std::tie(day_of_week, minutes) = GetParam();
  WeeklyTime weekly_time =
      WeeklyTime(day_of_week, minutes * kMinute.InMilliseconds());
  EXPECT_EQ(weekly_time.getDayOfWeek(), day_of_week);
  EXPECT_EQ(weekly_time.getMilliseconds(), minutes * kMinute.InMilliseconds());
}

TEST_P(WeeklyTimeConstructorAndToValue, ToValue) {
  int day_of_week;
  int minutes;
  std::tie(day_of_week, minutes) = GetParam();
  WeeklyTime weekly_time =
      WeeklyTime(day_of_week, minutes * kMinute.InMilliseconds());
  std::unique_ptr<base::DictionaryValue> weekly_time_value =
      weekly_time.ToValue();
  int day_of_week_value;
  weekly_time_value->GetInteger("day_of_week", &day_of_week_value);
  int milliseconds_value;
  weekly_time_value->GetInteger("time", &milliseconds_value);
  EXPECT_EQ(day_of_week_value, day_of_week);
  EXPECT_EQ(milliseconds_value, minutes * kMinute.InMilliseconds());
}

INSTANTIATE_TEST_CASE_P(TheSmallestCase,
                        WeeklyTimeConstructorAndToValue,
                        testing::Combine(testing::Values(1),
                                         testing::Values(0)));

INSTANTIATE_TEST_CASE_P(TheBiggestCase,
                        WeeklyTimeConstructorAndToValue,
                        testing::Combine(testing::Values(7),
                                         testing::Values(24 * 60 - 1)));

INSTANTIATE_TEST_CASE_P(RandomCase,
                        WeeklyTimeConstructorAndToValue,
                        testing::Combine(testing::Values(3),
                                         testing::Values(15 * 60 + 30)));

class WeeklyTimeGetDuration
    : public testing::Test,
      public testing::WithParamInterface<std::tuple<int, int, int, int>> {};

TEST_P(WeeklyTimeGetDuration, GetDuration) {
  int day1;
  int minutes1;
  int day2;
  int minutes2;
  std::tie(day1, minutes1, day2, minutes2) = GetParam();
  WeeklyTime weekly_time1 =
      WeeklyTime(day1, minutes1 * kMinute.InMilliseconds());
  WeeklyTime weekly_time2 =
      WeeklyTime(day2, minutes2 * kMinute.InMilliseconds());
  base::TimeDelta duration =
      (day2 - day1) * kDay + (minutes2 - minutes1) * kMinute;
  EXPECT_EQ(WeeklyTime::GetDuration(weekly_time1, weekly_time2), duration);
  if (!duration.is_zero())
    EXPECT_EQ(WeeklyTime::GetDuration(weekly_time2, weekly_time1),
              kWeek - duration);
}

INSTANTIATE_TEST_CASE_P(ZeroDuration,
                        WeeklyTimeGetDuration,
                        testing::Combine(testing::Values(3),
                                         testing::Values(60),
                                         testing::Values(3),
                                         testing::Values(60)));

INSTANTIATE_TEST_CASE_P(HourDuration,
                        WeeklyTimeGetDuration,
                        testing::Combine(testing::Values(4),
                                         testing::Values(54),
                                         testing::Values(4),
                                         testing::Values(60 + 54)));

INSTANTIATE_TEST_CASE_P(TheLongestDuration,
                        WeeklyTimeGetDuration,
                        testing::Combine(testing::Values(1),
                                         testing::Values(0),
                                         testing::Values(7),
                                         testing::Values(24 * 60 - 1)));

INSTANTIATE_TEST_CASE_P(RandomDuration,
                        WeeklyTimeGetDuration,
                        testing::Combine(testing::Values(2),
                                         testing::Values(15 * 60 + 30),
                                         testing::Values(5),
                                         testing::Values(17 * 60 + 45)));

class WeeklyTimeConvertToUTCTime
    : public testing::Test,
      public testing::WithParamInterface<std::tuple<int, int, int>> {};

TEST_P(WeeklyTimeConvertToUTCTime, ConvertToUTCTime) {
  int day_of_week;
  int minutes;
  int gmt_offset_minutes;
  std::tie(day_of_week, minutes, gmt_offset_minutes) = GetParam();
  int gmt_offset_positive = kWeek.InMinutes() - gmt_offset_minutes;
  base::TimeDelta time_utc = base::TimeDelta::FromDays(day_of_week) +
                             base::TimeDelta::FromMinutes(minutes) +
                             base::TimeDelta::FromMinutes(gmt_offset_positive);
  int day_of_week_utc = (time_utc.InDays() - 1) % 7 + 1;
  int milliseconds_utc = time_utc.InMilliseconds() % kDay.InMilliseconds();
  WeeklyTime weekly_time_utc =
      ConvertToUTCWeeklyTime(day_of_week, minutes * kMinute.InMilliseconds(),
                             gmt_offset_minutes * kMinute.InMilliseconds());
  EXPECT_EQ(weekly_time_utc.getDayOfWeek(), day_of_week_utc);
  EXPECT_EQ(weekly_time_utc.getMilliseconds(), milliseconds_utc);
}

INSTANTIATE_TEST_CASE_P(ZeroOffset,
                        WeeklyTimeConvertToUTCTime,
                        testing::Combine(testing::Values(2),
                                         testing::Values(15 * 60 + 30),
                                         testing::Values(0)));

INSTANTIATE_TEST_CASE_P(TheSmallestOffset,
                        WeeklyTimeConvertToUTCTime,
                        testing::Combine(testing::Values(2),
                                         testing::Values(15 * 60 + 30),
                                         testing::Values(-13 * 60)));

INSTANTIATE_TEST_CASE_P(TheSmallestOffsetToNextDay,
                        WeeklyTimeConvertToUTCTime,
                        testing::Combine(testing::Values(7),
                                         testing::Values(21 * 60 + 30),
                                         testing::Values(-13 * 60)));

INSTANTIATE_TEST_CASE_P(TheBiggestOffset,
                        WeeklyTimeConvertToUTCTime,
                        testing::Combine(testing::Values(3),
                                         testing::Values(15 * 60 + 30),
                                         testing::Values(13 * 60)));

INSTANTIATE_TEST_CASE_P(TheBiggestOffsetToPreviousDay,
                        WeeklyTimeConvertToUTCTime,
                        testing::Combine(testing::Values(1),
                                         testing::Values(9 * 60 + 30),
                                         testing::Values(-13 * 60)));

INSTANTIATE_TEST_CASE_P(HalfAnHourOffset,
                        WeeklyTimeConvertToUTCTime,
                        testing::Combine(testing::Values(3),
                                         testing::Values(10 * 60 + 47),
                                         testing::Values(5 * 60 + 30)));

INSTANTIATE_TEST_CASE_P(FifteenMinutesOffset,
                        WeeklyTimeConvertToUTCTime,
                        testing::Combine(testing::Values(1),
                                         testing::Values(10 * 60 + 47),
                                         testing::Values(6 * 60 + 15)));

INSTANTIATE_TEST_CASE_P(RandomOffset,
                        WeeklyTimeConvertToUTCTime,
                        testing::Combine(testing::Values(4),
                                         testing::Values(22 * 60 + 24),
                                         testing::Values(7 * 60)));

}  // namespace chromeos
