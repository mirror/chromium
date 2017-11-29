/*
 * Copyright (C) 2009 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *     * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
 * distribution.
 *     * Neither the name of Google Inc. nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "platform/wtf/Time.h"

#include "base/time/time.h"

namespace WTF {

// Store references rather than the callbacks themselves to avoid global
// destructors.
static TimeCallback* g_mock_time_callback = nullptr;
static TimeCallback* g_mock_monotonically_increasing_time_callback = nullptr;

double CurrentTime() {
  if (g_mock_time_callback)
    return g_mock_time_callback->Run();
  return base::Time::Now().ToDoubleT();
}

double MonotonicallyIncreasingTime() {
  if (g_mock_monotonically_increasing_time_callback)
    return g_mock_monotonically_increasing_time_callback->Run();
  return base::TimeTicks::Now().ToInternalValue() /
         static_cast<double>(base::Time::kMicrosecondsPerSecond);
}

TimeCallback* SetCurrentTimeCallbackForTesting(TimeCallback* new_callback) {
  DCHECK(!new_callback || *new_callback);
  TimeCallback* old_callback = g_mock_time_callback;
  g_mock_time_callback = new_callback;
  return old_callback;
}

TimeCallback* SetMonotonicallyIncreasingTimeCallbackForTesting(
    TimeCallback* new_callback) {
  DCHECK(!new_callback || *new_callback);
  TimeCallback* old_callback = g_mock_monotonically_increasing_time_callback;
  g_mock_monotonically_increasing_time_callback = new_callback;
  return old_callback;
}

TimeCallback* GetMonotonicallyIncreasingTimeCallbackForTesting() {
  return g_mock_monotonically_increasing_time_callback;
}

ScopedTimeFunctionsOverrideForTesting::ScopedTimeFunctionsOverrideForTesting(
    TimeCallback current_time_callback,
    TimeCallback monotonically_increasing_time_callback)
    : current_time_callback_(std::move(current_time_callback)),
      monotonically_increasing_time_callback_(
          std::move(monotonically_increasing_time_callback)) {
  if (current_time_callback_) {
    original_current_time_callback_ =
        SetCurrentTimeCallbackForTesting(&current_time_callback_);
  }
  if (monotonically_increasing_time_callback_) {
    original_monotonically_increasing_time_callback_ =
        SetMonotonicallyIncreasingTimeCallbackForTesting(
            &monotonically_increasing_time_callback_);
  }
}

// WTF::Function is not copyable, so convert to base::Callback and copy that.
ScopedTimeFunctionsOverrideForTesting::ScopedTimeFunctionsOverrideForTesting(
    TimeCallback current_and_monotonically_increasing_time_callback)
    : ScopedTimeFunctionsOverrideForTesting(ConvertToBaseCallback(
          std::move(current_and_monotonically_increasing_time_callback))) {}

ScopedTimeFunctionsOverrideForTesting::ScopedTimeFunctionsOverrideForTesting(
    base::Callback<double()> current_and_monotonically_increasing_time_callback)
    : ScopedTimeFunctionsOverrideForTesting(
          TimeCallback(current_and_monotonically_increasing_time_callback),
          TimeCallback(current_and_monotonically_increasing_time_callback)) {}

ScopedTimeFunctionsOverrideForTesting::
    ~ScopedTimeFunctionsOverrideForTesting() {
  if (current_time_callback_)
    SetCurrentTimeCallbackForTesting(original_current_time_callback_);
  if (monotonically_increasing_time_callback_) {
    SetMonotonicallyIncreasingTimeCallbackForTesting(
        original_monotonically_increasing_time_callback_);
  }
}

}  // namespace WTF
