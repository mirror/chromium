// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "media/gpu/android/coalescing_timer.h"

#include "base/callback.h"
#include "base/stl_util.h"

namespace media {
namespace {
const constexpr base::TimeDelta kTimerPeriod =
    base::TimeDelta::FromMilliseconds(10);
}

CoalescingTimer::CoalescingTimer() = default;

CoalescingTimer::~CoalescingTimer() {
  Stop();
}

void CoalescingTimer::Start(const base::Closure& callback) {
  bool was_running = !callback_.is_null();
  callback_ = callback;
  if (!was_running)
    Manager::Instance()->AddTimer(this);
}

void CoalescingTimer::Stop() {
  callback_.Reset();
  Manager::Instance()->RemoveTimer(this);
}

CoalescingTimer::Manager::Manager() = default;

CoalescingTimer::Manager* CoalescingTimer::Manager::Instance() {
  static Manager* manager = new Manager();
  return manager;
}

void CoalescingTimer::Manager::AddTimer(CoalescingTimer* timer) {
  DCHECK(thread_checker_.CalledOnValidThread());
  running_timers_.push_back(timer);
  if (timer_.IsRunning())
    return;
  timer_.Start(FROM_HERE, kTimerPeriod, this, &Manager::RunCallbacks);
}

void CoalescingTimer::Manager::RemoveTimer(CoalescingTimer* timer) {
  DCHECK(thread_checker_.CalledOnValidThread());
  base::Erase(running_timers_, timer);
}

void CoalescingTimer::Manager::RunCallbacks() {
  DCHECK(thread_checker_.CalledOnValidThread());
  // Iterate over a copy so that the timer callbacks can safely add/remove
  // timers.
  std::vector<CoalescingTimer*> timers(running_timers_);
  for (auto* timer : timers) {
    timer->callback().Run();
  }
}

}  // namespace media
