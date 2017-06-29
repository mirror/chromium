// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/synchronization/waitable_event.h"

#include <sys/event.h>

#include "base/debug/activity_tracker.h"
#include "base/logging.h"
#include "base/posix/eintr_wrapper.h"
#include "base/strings/stringprintf.h"
#include "base/threading/thread_restrictions.h"

namespace base {

WaitableEvent::WaitableEvent(ResetPolicy reset_policy,
                             InitialState initial_state)
    : kqueue_(kqueue()),
      dispatch_queue_(dispatch_queue_create(
          base::StringPrintf(
              "org.chromium.base.WaitableEvent.%p", this).c_str(),
          DISPATCH_QUEUE_SERIAL)),
      async_waiters_() {
  PCHECK(kqueue_.is_valid()) << "kqueue";
  uint16_t flags = EV_ADD;
  uint32_t fflags = 0;

  if (reset_policy == ResetPolicy::AUTOMATIC)
    flags |= EV_CLEAR;

  kevent64_s event = MakeEvent(flags, fflags);
  int rv = HANDLE_EINTR(
      kevent64(kqueue_.get(), &event, 1, nullptr, 0, 0, nullptr));
  PCHECK(rv == 0) << "kevent64: register";

  if (initial_state == InitialState::SIGNALED)
    Signal();
}

WaitableEvent::~WaitableEvent() = default;

void WaitableEvent::Reset() {
  kevent64_s event = MakeEvent(EV_DISABLE, 0);
  int rv = HANDLE_EINTR(
      kevent64(kqueue_.get(), &event, 1, nullptr, 0, 0, nullptr));
  PCHECK(rv == 0) << "kevent64: Reset";
}

void WaitableEvent::Signal() {
  // Wake the async watchers first, because the synchronous watcher may delete
  // it immediately after being woken from kevent. That would lead to a
  // situation where the signaler is still trying to read and invoke the
  // async awaiters when the object is deleted.
  DispatchAsyncWatchers();

  kevent64_s event = MakeEvent(EV_ENABLE, NOTE_TRIGGER);
  int rv = HANDLE_EINTR(
      kevent64(kqueue_.get(), &event, 1, nullptr, 0, 0, nullptr));
  PCHECK(rv == 0) << "kevent64: Signal";
}

bool WaitableEvent::IsSignaled() {
  // TODO(rsesek): Use KEVENT_FLAG_IMMEDIATE rather than an empty timeout.
  timespec ts{};
  kevent64_s event;
  int rv = kevent64(kqueue_.get(), nullptr, 0, &event, 1, 0, &ts);
  PCHECK(rv >= 0) << "kevent64 IsSignaled";
  return rv > 0;
}

void WaitableEvent::Wait() {
  bool result = TimedWaitUntil(TimeTicks::Max());
  DCHECK(result) << "TimedWait() should never fail with infinite timeout";
}

bool WaitableEvent::TimedWait(const TimeDelta& wait_delta) {
  return TimedWaitUntil(TimeTicks::Now() + wait_delta);
}

bool WaitableEvent::TimedWaitUntil(const TimeTicks& end_time) {
  base::ThreadRestrictions::AssertWaitAllowed();
  // Record the event that this thread is blocking upon (for hang diagnosis).
  base::debug::ScopedEventWaitActivity event_activity(this);

  bool indefinite = end_time.is_max();

  TimeDelta wait_time = end_time - TimeTicks::Now();
  if (wait_time < TimeDelta()) {
    wait_time = TimeDelta();
  }

  timespec timeout = wait_time.ToTimeSpec();

  kevent64_s event;
  int rv = HANDLE_EINTR(kevent64(kqueue_.get(), nullptr, 0, &event, 1, 0,
      indefinite ? nullptr : &timeout));
  PCHECK(rv >= 0) << "kevent64 TimedWait";
  return rv > 0;
}

// static
size_t WaitableEvent::WaitMany(WaitableEvent** raw_waitables,
                               size_t count) {
  base::ThreadRestrictions::AssertWaitAllowed();
  DCHECK(count) << "Cannot wait on no events";

  // Record an event (the first) that this thread is blocking upon.
  base::debug::ScopedEventWaitActivity event_activity(raw_waitables[0]);

  std::vector<kevent64_s> events(count);
  for (size_t i = 0; i < count; ++i) {
    EV_SET64(&events[i], raw_waitables[i]->kqueue_.get(),
        EVFILT_READ, EV_ADD | EV_CLEAR, 0, 0, i, 0, 0);
  }

  std::vector<kevent64_s> out_events(count);

  base::ScopedFD wait_main_kqueue(kqueue());
  PCHECK(wait_main_kqueue.is_valid()) << "kqueue WaitMany";

  int rv = HANDLE_EINTR(kevent64(wait_main_kqueue.get(), events.data(), count,
      out_events.data(), count, 0, nullptr));
  PCHECK(rv > 0) << "kevent64: WaitMany";

  size_t triggered = -1;
  for (size_t i = 0; i < static_cast<size_t>(rv); ++i) {
    // WaitMany should return the lowest index in |raw_waitables| that was
    // triggered.
    size_t index = static_cast<size_t>(out_events[i].udata);
    triggered = std::min(triggered, index);
  }

  // The WaitMany kevent has identified which kqueue was signaled. Trigger
  // a Wait on it to clear the event from within WaitableEvent's kqueue. This
  // will not block, since it has been triggered.
  raw_waitables[triggered]->Wait();

  return triggered;
}

void WaitableEvent::AddWatcherSource(dispatch_source_t source) {
  dispatch_sync(dispatch_queue_, ^{
    async_waiters_.emplace_back(source, base::scoped_policy::RETAIN);
  });
  if (IsSignaled())
    DispatchAsyncWatchers();
}

void WaitableEvent::DispatchAsyncWatchers() {
  dispatch_sync(dispatch_queue_, ^{
    for (auto it = async_waiters_.begin(); it != async_waiters_.end();) {
      if (dispatch_source_testcancel(*it)) {
        async_waiters_.erase(it);
      } else {
        dispatch_source_merge_data(*it, 1);
        ++it;
      }
    }
  });
}

kevent64_s WaitableEvent::MakeEvent(uint16_t flags, uint32_t fflags) {
  kevent64_s event;
  EV_SET64(&event, reinterpret_cast<uint64_t>(this), EVFILT_USER, flags,
      fflags, 0, 0, 0, 0);
  return event;
}

}  // namespace base
