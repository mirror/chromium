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

namespace {

constexpr base::TimeDelta kMinute = base::TimeDelta::FromMinutes(1);

}  // namespace

class IntervalTest : public testing::Test {
 protected:
  IntervalTest() {}
};

TEST_F(IntervalTest, Constructor) {
  for (int day1 = 1; day1 < 8; day1++) {
    for (int minutes1 = 0; minutes1 < 24 * 60; minutes1++) {
      for (int day2 = day1; day2 < 8; day2++) {
        for (int minutes2 = minutes1; minutes2 < 24 * 60; minutes2++) {
          WeeklyTime weekly_time1 =
              WeeklyTime(day1, minutes1 * kMinute.InMilliseconds());
          WeeklyTime weekly_time2 =
              WeeklyTime(day2, minutes2 * kMinute.InMilliseconds());
          Interval interval1 = Interval(weekly_time1, weekly_time2);
          EXPECT_EQ(interval1.getStart().getDayOfWeek(), day1);
          EXPECT_EQ(interval1.getStart().getMilliseconds(),
                    minutes1 * kMinute.InMilliseconds());
          EXPECT_EQ(interval1.getEnd().getDayOfWeek(), day2);
          EXPECT_EQ(interval1.getEnd().getMilliseconds(),
                    minutes2 * kMinute.InMilliseconds());
          Interval interval2 = Interval(weekly_time2, weekly_time1);
          EXPECT_EQ(interval2.getStart().getDayOfWeek(), day2);
          EXPECT_EQ(interval2.getStart().getMilliseconds(),
                    minutes2 * kMinute.InMilliseconds());
          EXPECT_EQ(interval2.getEnd().getDayOfWeek(), day1);
          EXPECT_EQ(interval2.getEnd().getMilliseconds(),
                    minutes1 * kMinute.InMilliseconds());
        }
      }
    }
  }
}

// TODO TEST_F(IntervalTest, ToValue)

TEST_F(IntervalTest, Contains) {
  for (int day1 = 1; day1 < 8; day1++) {
    for (int minutes1 = 0; minutes1 < 24 * 60; minutes1 += 15) {
      WeeklyTime weekly_time1 =
          WeeklyTime(day1, minutes1 * kMinute.InMilliseconds());
      for (int day2 = day1; day2 < 8; day2++) {
        for (int minutes2 = minutes1 + 1; minutes2 < 24 * 60; minutes2 += 15) {
          WeeklyTime weekly_time2 =
              WeeklyTime(day2, minutes2 * kMinute.InMilliseconds());
          for (int day3 = day2; day3 < 8; day3++) {
            for (int minutes3 = minutes2 + 1; minutes3 < 24 * 60;
                 minutes3 += 15) {
              WeeklyTime weekly_time3 =
                  WeeklyTime(day3, minutes3 * kMinute.InMilliseconds());

              EXPECT_TRUE(
                  Interval(weekly_time3, weekly_time2).Contains(weekly_time1));
              EXPECT_FALSE(
                  Interval(weekly_time2, weekly_time3).Contains(weekly_time1));
              EXPECT_TRUE(
                  Interval(weekly_time1, weekly_time3).Contains(weekly_time2));
              EXPECT_FALSE(
                  Interval(weekly_time3, weekly_time1).Contains(weekly_time2));
              EXPECT_TRUE(
                  Interval(weekly_time2, weekly_time1).Contains(weekly_time3));
              EXPECT_FALSE(
                  Interval(weekly_time1, weekly_time2).Contains(weekly_time3));
            }
          }
          EXPECT_TRUE(
              Interval(weekly_time1, weekly_time2).Contains(weekly_time1));
          EXPECT_FALSE(
              Interval(weekly_time2, weekly_time1).Contains(weekly_time1));
          EXPECT_TRUE(
              Interval(weekly_time2, weekly_time1).Contains(weekly_time2));
          EXPECT_FALSE(
              Interval(weekly_time1, weekly_time2).Contains(weekly_time2));
        }
      }
      EXPECT_FALSE(Interval(weekly_time1, weekly_time1).Contains(weekly_time1));
    }
  }
}

}  // namespace chromeos
