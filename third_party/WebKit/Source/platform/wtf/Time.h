// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef WTF_Time_h
#define WTF_Time_h

#include "base/callback.h"
#include "base/time/time.h"
#include "platform/wtf/Functional.h"
#include "platform/wtf/WTFExport.h"

namespace WTF {
// Provides thin wrappers around the following basic time types from
// base/time package:
//
//  - WTF::TimeDelta is an alias for base::TimeDelta and represents a duration
//    of time.
//  - WTF::TimeTicks wraps base::TimeTicks and represents a monotonic time
//    value.
//  - WTF::Time is an alias for base::Time and represents a wall time value.
//
// For usage guideline please see the documentation in base/time/time.h

using TimeDelta = base::TimeDelta;
using Time = base::Time;

// Returns the current UTC time in seconds, counted from January 1, 1970.
// Precision varies depending on platform but is usually as good or better
// than a millisecond.
WTF_EXPORT double CurrentTime();

// Same thing, in milliseconds.
inline double CurrentTimeMS() {
  return CurrentTime() * 1000.0;
}

// Provides a monotonically increasing time in seconds since an arbitrary point
// in the past.  On unsupported platforms, this function only guarantees the
// result will be non-decreasing.
WTF_EXPORT double MonotonicallyIncreasingTime();

// Same thing, in milliseconds.
inline double MonotonicallyIncreasingTimeMS() {
  return MonotonicallyIncreasingTime() * 1000.0;
}

using TimeFunction = double (*)();
using TimeCallback = WTF::RepeatingFunction<double()>;

// Make CurrentTime() and CurrentTimeMs() return the result of the supplied
// callback. Returns the pointer to the old time callback. |nullptr| disables
// the override. Caller maintains ownership of callback.
WTF_EXPORT TimeCallback* SetCurrentTimeCallbackForTesting(TimeCallback*);

// Make MonotonicallyIncreasingTime() and MonotonicallyIncreasingTimeMs() return
// the result of the supplied callback. Returns the pointer to the old time
// callback. |nullptr| disables the override. Caller maintains ownership of
// callback.
WTF_EXPORT TimeCallback* SetMonotonicallyIncreasingTimeCallbackForTesting(
    TimeCallback*);

// Allows wtf/Time.h to use the same mock callback.
WTF_EXPORT TimeCallback* GetMonotonicallyIncreasingTimeCallbackForTesting();

class WTF_EXPORT ScopedTimeFunctionsOverrideForTesting {
 public:
  ScopedTimeFunctionsOverrideForTesting(
      TimeCallback current_time_callback,
      TimeCallback monotonically_increasing_time_callback);
  // Helper to facilitate overriding both functions with the same callback.
  ScopedTimeFunctionsOverrideForTesting(
      TimeCallback current_and_monotonically_increasing_time_callback);
  ~ScopedTimeFunctionsOverrideForTesting();

 private:
  ScopedTimeFunctionsOverrideForTesting(
      base::Callback<double()>
          current_and_monotonically_increasing_time_callback);

  TimeCallback current_time_callback_;
  TimeCallback monotonically_increasing_time_callback_;
  TimeCallback* original_current_time_callback_;
  TimeCallback* original_monotonically_increasing_time_callback_;
};

class TimeTicks {
 public:
  TimeTicks() {}
  TimeTicks(base::TimeTicks value) : value_(value) {}

  static TimeTicks Now() {
    if (GetMonotonicallyIncreasingTimeCallbackForTesting()) {
      double seconds =
          GetMonotonicallyIncreasingTimeCallbackForTesting()->Run();
      return TimeTicks() + TimeDelta::FromSecondsD(seconds);
    }
    return TimeTicks(base::TimeTicks::Now());
  }

  static TimeTicks UnixEpoch() {
    return TimeTicks(base::TimeTicks::UnixEpoch());
  }

  int64_t ToInternalValueForTesting() const { return value_.ToInternalValue(); }

  // Only use this conversion when interfacing with legacy code that represents
  // time in double. Converting to double can lead to losing information for
  // large time values.
  double InSeconds() const { return (value_ - base::TimeTicks()).InSecondsF(); }

  static TimeTicks FromSeconds(double seconds) {
    return TimeTicks() + TimeDelta::FromSecondsD(seconds);
  }

  operator base::TimeTicks() const { return value_; }

  TimeTicks& operator=(TimeTicks other) {
    value_ = other.value_;
    return *this;
  }

  TimeDelta operator-(TimeTicks other) const { return value_ - other.value_; }

  TimeTicks operator+(TimeDelta delta) const {
    return TimeTicks(value_ + delta);
  }
  TimeTicks operator-(TimeDelta delta) const {
    return TimeTicks(value_ - delta);
  }

  TimeTicks& operator+=(TimeDelta delta) {
    value_ += delta;
    return *this;
  }
  TimeTicks& operator-=(TimeDelta delta) {
    value_ -= delta;
    return *this;
  }

  bool operator==(TimeTicks other) const { return value_ == other.value_; }
  bool operator!=(TimeTicks other) const { return value_ != other.value_; }
  bool operator<(TimeTicks other) const { return value_ < other.value_; }
  bool operator<=(TimeTicks other) const { return value_ <= other.value_; }
  bool operator>(TimeTicks other) const { return value_ > other.value_; }
  bool operator>=(TimeTicks other) const { return value_ >= other.value_; }

 private:
  base::TimeTicks value_;
};

}  // namespace WTF

using WTF::CurrentTime;
using WTF::CurrentTimeMS;
using WTF::MonotonicallyIncreasingTime;
using WTF::MonotonicallyIncreasingTimeMS;
using WTF::ScopedTimeFunctionsOverrideForTesting;
using WTF::SetCurrentTimeCallbackForTesting;
using WTF::SetMonotonicallyIncreasingTimeCallbackForTesting;
using WTF::Time;
using WTF::TimeCallback;
using WTF::TimeDelta;
using WTF::TimeFunction;
using WTF::TimeTicks;

#endif  // Time_h
