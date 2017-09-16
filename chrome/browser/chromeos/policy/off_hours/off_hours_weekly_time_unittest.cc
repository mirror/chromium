// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/chromeos/policy/off_hours/off_hours_weekly_time.h"

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

class WeeklyTimeTest : public testing::Test {
 protected:
  WeeklyTimeTest() {}
};

TEST_F(WeeklyTimeTest, Constructor) {
  for (int day_of_week = 0; day_of_week < 8; day_of_week++) {
    for (int minutes = 0; minutes <= 24 * 60; minutes++) {
      if (day_of_week > 0 && minutes != 24 * 60) {
        WeeklyTime weekly_time =
            WeeklyTime(day_of_week, minutes * kMinute.InMilliseconds());
        EXPECT_EQ(weekly_time.getDayOfWeek(), day_of_week);
        EXPECT_EQ(weekly_time.getMilliseconds(),
                  minutes * kMinute.InMilliseconds());
      } else {
        //        ASSERT_TRUE(WeeklyTime(day_of_week, minutes *
        //        kMinute.InMilliseconds()));
      }
    }
  }
}

TEST_F(WeeklyTimeTest, ToValue) {
  for (int day_of_week = 1; day_of_week < 8; day_of_week++) {
    for (int minutes = 0; minutes < 24 * 60; minutes++) {
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
  }
}

TEST_F(WeeklyTimeTest, GetDuration) {
  for (int day1 = 1; day1 < 8; day1++) {
    for (int minutes1 = 0; minutes1 < 24 * 60; minutes1++) {
      for (int day2 = day1; day2 < 8; day2++) {
        for (int minutes2 = minutes1; minutes2 < 24 * 60; minutes2++) {
          WeeklyTime weekly_time1 =
              WeeklyTime(day1, minutes1 * kMinute.InMilliseconds());
          WeeklyTime weekly_time2 =
              WeeklyTime(day2, minutes2 * kMinute.InMilliseconds());
          base::TimeDelta duration =
              (day2 - day1) * kDay + (minutes2 - minutes1) * kMinute;
          EXPECT_EQ(WeeklyTime::GetDuration(weekly_time1, weekly_time2),
                    duration);
          if (!duration.is_zero())
            EXPECT_EQ(WeeklyTime::GetDuration(weekly_time2, weekly_time1),
                      kWeek - duration);
        }
      }
    }
  }
}

TEST_F(WeeklyTimeTest, ConvertToUTCTime) {
  for (int day_of_week = 1; day_of_week < 8; day_of_week++) {
    for (int minutes = 0; minutes < 24 * 60; minutes++) {
      for (int gmt_offset_minutes = -13 * 60; gmt_offset_minutes <= 13 * 60;
           gmt_offset_minutes += 15) {
        int gmt_offset_positive = kWeek.InMinutes() - gmt_offset_minutes;
        base::TimeDelta time_utc =
            base::TimeDelta::FromDays(day_of_week) +
            base::TimeDelta::FromMinutes(minutes) +
            base::TimeDelta::FromMinutes(gmt_offset_positive);
        int day_of_week_utc = (time_utc.InDays() - 1) % 7 + 1;
        int milliseconds_utc =
            time_utc.InMilliseconds() % kDay.InMilliseconds();
        WeeklyTime weekly_time_utc = ConvertToUTCWeeklyTime(
            day_of_week, minutes * kMinute.InMilliseconds(),
            gmt_offset_minutes * kMinute.InMilliseconds());
        EXPECT_EQ(weekly_time_utc.getDayOfWeek(), day_of_week_utc);
        EXPECT_EQ(weekly_time_utc.getMilliseconds(), milliseconds_utc);
      }
    }
  }
}

}  // namespace chromeos
