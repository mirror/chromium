// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_CHROMEOS_POLICY_OFF_HOURS_WEEKLY_TIME_H_
#define CHROME_BROWSER_CHROMEOS_POLICY_OFF_HOURS_WEEKLY_TIME_H_

#include <memory>

#include "base/memory/ptr_util.h"
#include "base/time/time.h"
#include "base/values.h"

namespace policy {

// WeeklyTime class contains day of week and time. Day of week is number from 1
// to 7 (1 = Monday, 2 = Tuesday, etc.) Time is in milliseconds from the
// beginning of the day.
class WeeklyTime {
 public:
  WeeklyTime(int day_of_week, int milliseconds);

  // It's used by ConvertOffHoursProtoToValue(..) function for conversion from
  // proto structure to PolicyMap.
  std::unique_ptr<base::DictionaryValue> ToValue() const;

  int day_of_week() const { return day_of_week_; }

  int milliseconds() const { return milliseconds_; }

  // Return duration from |start| till |end| week times. |end| time
  // is always after |start| time. It's possible because week time is cyclic.
  // (i.e. [Friday 17:00, Monday 9:00) )
  static base::TimeDelta GetDuration(const WeeklyTime& start,
                                     const WeeklyTime& end);

 private:
  // Number of weekday (1 = Monday, 2 = Tuesday, etc.)
  int day_of_week_;

  // Time of day in milliseconds from the beginning of the day.
  int milliseconds_;
};

}  // namespace policy

#endif  // CHROME_BROWSER_CHROMEOS_POLICY_OFF_HOURS_WEEKLY_TIME_H_
