// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/time/time.h"

#include <zircon/syscalls.h>

#include "base/compiler_specific.h"
#include "base/numerics/checked_math.h"
#include "base/time/time_override.h"

namespace base {

namespace {

// Helper function to map an unsigned integer with nanosecond timebase to a
// signed integer with microsecond timebase.
ALWAYS_INLINE int64_t ZxTimeToMicroseconds(zx_time_t nanos) {
  const zx_time_t micros =
      nanos / static_cast<zx_time_t>(base::Time::kNanosecondsPerMicrosecond);
  return static_cast<int64_t>(micros);
}

}  // namespace

// Time -----------------------------------------------------------------------
namespace {
TimeNowFunction g_time_now_function = &TimeNowIgnoringOverride;
}  // namespace

TimeNowFunction SetTimeClockOverride(TimeNowFunction func_ptr) {
  auto original = g_time_now_function;
  g_time_now_function = func_ptr;
  return original;
}

// static
Time Time::Now() {
  return g_time_now_function();
}

// static
Time Time::NowFromSystemTime() {
  // Just use g_time_now_function because it returns the system time.
  return g_time_now_function();
}

Time TimeNowIgnoringOverride() {
  const zx_time_t nanos_since_unix_epoch = zx_time_get(ZX_CLOCK_UTC);
  CHECK(nanos_since_unix_epoch != 0);
  // The following expression will overflow in the year 289938 A.D.:
  return Time(ZxTimeToMicroseconds(nanos_since_unix_epoch) +
              Time::kTimeTToMicrosecondsOffset);
}

Time TimeNowFromSystemTimeIgnoringOverride() {
  // Just use TimeNowIgnoringOverride() because it returns the system time.
  return TimeNowIgnoringOverride();
}

// TimeTicks ------------------------------------------------------------------
namespace {
TimeTicksNowFunction g_time_ticks_now_function = &TimeTicksNowIgnoringOverride;
}  // namespace

TimeTicksNowFunction SetTimeTicksClockOverride(TimeTicksNowFunction func_ptr) {
  auto original = g_time_ticks_now_function;
  g_time_ticks_now_function = func_ptr;
  return original;
}

// static
TimeTicks TimeTicks::Now() {
  return g_time_ticks_now_function();
}

TimeTicks TimeTicksNowIgnoringOverride() {
  const zx_time_t nanos_since_boot = zx_time_get(ZX_CLOCK_MONOTONIC);
  CHECK(nanos_since_boot != 0);
  return TimeTicks(ZxTimeToMicroseconds(nanos_since_boot));
}

// static
TimeTicks::Clock TimeTicks::GetClock() {
  return Clock::FUCHSIA_ZX_CLOCK_MONOTONIC;
}

// static
bool TimeTicks::IsHighResolution() {
  return true;
}

// static
bool TimeTicks::IsConsistentAcrossProcesses() {
  return true;
}

// static
TimeTicks TimeTicks::FromZxTime(zx_time_t nanos_since_boot) {
  return TimeTicks(ZxTimeToMicroseconds(nanos_since_boot));
}

zx_time_t TimeTicks::ToZxTime() const {
  CheckedNumeric<zx_time_t> result(base::Time::kNanosecondsPerMicrosecond);
  result *= us_;
  return result.ValueOrDie();
}

// ThreadTicks ----------------------------------------------------------------
namespace {
ThreadTicksNowFunction g_thread_ticks_now_function =
    &ThreadTicksNowIgnoringOverride;
}  // namespace

ThreadTicksNowFunction SetThreadTicksClockOverride(
    ThreadTicksNowFunction func_ptr) {
  auto original = g_thread_ticks_now_function;
  g_thread_ticks_now_function = func_ptr;
  return original;
}

// static
ThreadTicks ThreadTicks::Now() {
  return g_thread_ticks_now_function();
}

ThreadTicks ThreadTicksNowIgnoringOverride() {
  const zx_time_t nanos_since_thread_started = zx_time_get(ZX_CLOCK_THREAD);
  CHECK(nanos_since_thread_started != 0);
  return ThreadTicks(ZxTimeToMicroseconds(nanos_since_thread_started));
}

}  // namespace base
