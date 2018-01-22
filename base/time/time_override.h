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
// progression of time with Blink's virtual time. Note that the override must be
// set before first call to Now().
//
// Pass |nullptr| to clear the override. The methods return a pointer to the
// previously set override or original now function if no override was in place.
BASE_EXPORT TimeNowFunction SetTimeClockOverride(TimeNowFunction func_ptr);
BASE_EXPORT TimeTicksNowFunction
SetTimeTicksClockOverride(TimeTicksNowFunction func_ptr);
BASE_EXPORT ThreadTicksNowFunction
SetThreadTicksClockOverride(ThreadTicksNowFunction func_ptr);

// These methods return the original Time::Now / TimeTicks::Now /
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
