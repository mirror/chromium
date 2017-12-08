// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "core/dom/IdleDeadline.h"

#include "core/timing/PerformanceBase.h"
#include "platform/scheduler/child/web_scheduler.h"
#include "platform/wtf/Time.h"
#include "public/platform/Platform.h"

namespace blink {

IdleDeadline::IdleDeadline(TimeTicks deadline, CallbackType callback_type)
    : deadline_(deadline), callback_type_(callback_type) {}

double IdleDeadline::timeRemaining() const {
  TimeDelta time_remaining = deadline_ - TimeTicks::Now();
  if (time_remaining < TimeDelta()) {
    time_remaining = TimeDelta();
  } else if (Platform::Current()
                 ->CurrentThread()
                 ->Scheduler()
                 ->ShouldYieldForHighPriorityWork()) {
    time_remaining = TimeDelta();
  }

  return PerformanceBase::ClampTimeResolution(time_remaining).InMillisecondsF();
}

}  // namespace blink
