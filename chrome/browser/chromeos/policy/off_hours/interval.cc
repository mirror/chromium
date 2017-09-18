// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/chromeos/policy/off_hours/interval.h"
#include "base/time/time.h"

namespace policy {

Interval::Interval(const WeeklyTime& start, const WeeklyTime& end)
    : start_(start), end_(end) {}

// It's used by ConvertOffHoursProtoToValue(..) function for conversion from
// proto structure to PolicyMap.
std::unique_ptr<base::DictionaryValue> Interval::ToValue() const {
  auto interval = base::MakeUnique<base::DictionaryValue>();
  interval->SetDictionary("start", start_.ToValue());
  interval->SetDictionary("end", end_.ToValue());
  return interval;
}

// Check if |w| is in [interval.start, interval.end). |end| time is always
// after |start| time. It's possible because week time is cyclic. (i.e.
// [Friday 17:00, Monday 9:00) )
bool Interval::Contains(const WeeklyTime& w) const {
  base::TimeDelta interval_duration = WeeklyTime::GetDuration(start_, end_);
  if (WeeklyTime::GetDuration(w, end_).is_zero())
    return false;
  return WeeklyTime::GetDuration(start_, w) +
             WeeklyTime::GetDuration(w, end_) ==
         interval_duration;
}

}  // namespace policy
