// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/chromeos/policy/off_hours/weekly_time.h"

namespace policy {
namespace {

constexpr base::TimeDelta kDay = base::TimeDelta::FromDays(1);
constexpr base::TimeDelta kWeek = base::TimeDelta::FromDays(7);

}  // namespace

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
      (end.day_of_week() - start.day_of_week()) * kDay.InMilliseconds() +
      end.milliseconds() - start.milliseconds();
  if (duration < 0)
    duration += kWeek.InMilliseconds();
  return base::TimeDelta::FromMilliseconds(duration);
}

}  // namespace policy
