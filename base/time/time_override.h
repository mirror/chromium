// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef BASE_TIME_TIME_OVERRIDE_H_
#define BASE_TIME_TIME_OVERRIDE_H_

#include "base/base_export.h"
#include "base/time/time.h"

namespace base {

using TimeNowFunction = decltype(&Time::Now);
using TimeTicksNowFunction = decltype(&TimeTicks::Now);
using ThreadTicksNowFunction = decltype(&ThreadTicks::Now);

// Override the return value of Time::Now and Time::NowFromSystemTime /
// TimeTicks::Now / ThreadTicks::Now to emulate time, e.g. for tests or to align
// progression of time with Blink's virtual time. Note that the override should
// be set while single-threaded and before the first call to Now() to avoid
// threading issues and inconsistencies in returned values.
//
// This is for headless (virtual time) and base_unittests use only. DO NOT use
// this for other use cases without first discussing with //base/time OWNERS.
class BASE_EXPORT ScopedTimeClockOverrides {
 public:
  // Pass |nullptr| for any override if it shouldn't be overriden.
  ScopedTimeClockOverrides(TimeNowFunction time_override,
                           TimeTicksNowFunction time_ticks_override,
                           ThreadTicksNowFunction thread_ticks_override);

  // Restores the platform default Now() functions.
  ~ScopedTimeClockOverrides();

 private:
  // Set overrides for individual Now() functions. The methods return a pointer
  // to the previously set override or platform default Now() function if no
  // override was in place.
  static TimeNowFunction SetTimeClockOverride(TimeNowFunction func_ptr);
  static TimeTicksNowFunction SetTimeTicksClockOverride(
      TimeTicksNowFunction func_ptr);
  static ThreadTicksNowFunction SetThreadTicksClockOverride(
      ThreadTicksNowFunction func_ptr);

  TimeNowFunction default_time_now_ = nullptr;
  TimeTicksNowFunction default_time_ticks_now_ = nullptr;
  ThreadTicksNowFunction default_thread_ticks_now_ = nullptr;

  DISALLOW_IMPLICIT_CONSTRUCTORS(ScopedTimeClockOverrides);
};

// These methods return the platform default Time::Now / TimeTicks::Now /
// ThreadTicks::Now values even while an override is in place. These methods
// should only be used in places where emulated time should be disregarded. For
// example, they can be used to implement test timeouts for tests that may
// override time.
BASE_EXPORT Time TimeNowIgnoringOverride();
BASE_EXPORT Time TimeNowFromSystemTimeIgnoringOverride();
BASE_EXPORT TimeTicks TimeTicksNowIgnoringOverride();
BASE_EXPORT ThreadTicks ThreadTicksNowIgnoringOverride();

}  // namespace base

#endif  // BASE_TIME_TIME_OVERRIDE_H_
