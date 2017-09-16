// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/chromeos/policy/off_hours/off_hours_weekly_time.h"

namespace policy {
namespace weekly_time {

constexpr base::TimeDelta kDay = base::TimeDelta::FromDays(1);
constexpr base::TimeDelta kWeek = base::TimeDelta::FromDays(7);

WeeklyTime::WeeklyTime(int day_of_week, int milliseconds)
    : day_of_week_(day_of_week), milliseconds_(milliseconds) {
  DCHECK_GT(day_of_week, 0);
  DCHECK_LE(day_of_week, 7);
  DCHECK_GE(milliseconds, 0);
  DCHECK_LT(milliseconds, kDay.InMilliseconds());
}

std::unique_ptr<base::DictionaryValue> WeeklyTime::ToValue() const {
  auto weekly_time = base::MakeUnique<base::DictionaryValue>();
  weekly_time->SetInteger("day_of_week", day_of_week_);
  weekly_time->SetInteger("time", milliseconds_);
  return weekly_time;
}

// static
base::TimeDelta WeeklyTime::GetDuration(const WeeklyTime& start,
                                        const WeeklyTime& end) {
  int duration =
      (end.getDayOfWeek() - start.getDayOfWeek()) * kDay.InMilliseconds() +
      end.getMilliseconds() - start.getMilliseconds();
  if (duration < 0)
    duration += kWeek.InMilliseconds();
  return base::TimeDelta::FromMilliseconds(duration);
}

WeeklyTime ConvertToUTCWeeklyTime(int day_of_week,
                                  int milliseconds,
                                  int gmt_offset) {
  // Convert time in milliseconds to GMT time considering day offset.
  int time_in_gmt = milliseconds - gmt_offset;
  // Make |time_in_gmt| positive number (add number of milliseconds per week)
  // for easier evaluation.
  time_in_gmt += kWeek.InMilliseconds();
  // Get milliseconds from the start of the day.
  int milliseconds_in_gmt = time_in_gmt % kDay.InMilliseconds();
  int day_offset = time_in_gmt / kDay.InMilliseconds();
  // Convert day of week to GMT timezone considering week is cyclic. +/- 1 is
  // because day of week is from 1 to 7.
  int day_of_week_in_gmt = (day_of_week + day_offset - 1) % 7 + 1;
  return WeeklyTime(day_of_week_in_gmt, milliseconds_in_gmt);
}

}  // namespace weekly_time
}  // namespace policy
