// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_CHROMEOS_POLICY_OFF_HOURS_INTERVAL_H_
#define CHROME_BROWSER_CHROMEOS_POLICY_OFF_HOURS_INTERVAL_H_

#include <memory>

#include "base/memory/ptr_util.h"
#include "base/values.h"
#include "chrome/browser/chromeos/policy/off_hours/weekly_time.h"

namespace policy {

// Time interval struct, interval = [start, end).
class Interval {
 public:
  Interval(const WeeklyTime& start, const WeeklyTime& end);

  // It's used by ConvertOffHoursProtoToValue(..) function for conversion from
  // proto structure to PolicyMap.
  std::unique_ptr<base::DictionaryValue> ToValue() const;

  // Check if |w| is in [interval.start, interval.end). |end| time is always
  // after |start| time. It's possible because week time is cyclic. (i.e.
  // [Friday 17:00, Monday 9:00) )
  bool Contains(const WeeklyTime& w) const;

  WeeklyTime start() const { return start_; }

  WeeklyTime end() const { return end_; }

 private:
  WeeklyTime start_;
  WeeklyTime end_;
};

}  // namespace policy

#endif  // CHROME_BROWSER_CHROMEOS_POLICY_OFF_HOURS_INTERVAL_H_
