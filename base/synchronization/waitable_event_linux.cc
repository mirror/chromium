// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/synchronization/waitable_event.h"

#include <poll.h>
#include <stddef.h>
#include <unistd.h>

#include "base/debug/activity_tracker.h"
#include "base/logging.h"
#include "base/posix/eintr_wrapper.h"
#include "base/threading/thread_restrictions.h"

namespace base {

WaitableEvent::WaitableEvent(ResetPolicy reset_policy,
                             InitialState initial_state)
    : event_fd_(eventfd(initial_state == InitialState::SIGNALED ? 1 : 0,
                        EFD_CLOEXEC | EFD_NONBLOCK)),
      auto_reset_(reset_policy == ResetPolicy::AUTOMATIC) {
  PCHECK(event_fd_.is_valid()) << "eventfd";
}

WaitableEvent::~WaitableEvent() = default;

void WaitableEvent::Reset() {
  ClearEvent();
}

void WaitableEvent::Signal() {
  uint64_t signal = 1;
  ssize_t rv = HANDLE_EINTR(write(event_fd_.get(), &signal, sizeof(signal)));
  PCHECK(rv == sizeof(signal)) << "eventfd Signal";
}

bool WaitableEvent::IsSignaled() {
  pollfd pfd{event_fd_.get(), POLLIN, 0};
  int rv = HANDLE_EINTR(poll(&pfd, 1, 0));
  if (rv <= 0) {
    PLOG_IF(ERROR, rv < 0) << "eventfd IsSignaled";
    return false;
  } else if ((pfd.revents & POLLIN) != 0) {
    if (auto_reset_) {
      ClearEvent();
    }
    return true;
  } else {
    NOTREACHED();
    return false;
  }
}

void WaitableEvent::Wait() {
  bool result = TimedWaitUntil(TimeTicks::Max());
  DCHECK(result) << "TimedWait() should never fail with infinite timeout";
}

bool WaitableEvent::TimedWait(const TimeDelta& wait_delta) {
  return TimedWaitUntil(TimeTicks::Now() + wait_delta);
}

bool WaitableEvent::TimedWaitUntil(const TimeTicks& end_time) {
  ThreadRestrictions::AssertWaitAllowed();
  debug::ScopedEventWaitActivity event_activity(this);

  bool indefinite = end_time.is_max();

  TimeDelta wait_time = end_time - TimeTicks::Now();
  if (wait_time < TimeDelta()) {
    wait_time = TimeDelta();
  }

  pollfd pfd{event_fd_.get(), POLLIN, 0};
  timespec timeout = wait_time.ToTimeSpec();

  int rv =
      HANDLE_EINTR(ppoll(&pfd, 1, indefinite ? nullptr : &timeout, nullptr));
  if (rv <= 0) {
    PLOG_IF(ERROR, rv < 0) << "eventfd ppoll";
    return false;
  }

  if (pfd.revents & POLLIN) {
    if (auto_reset_) {
      ClearEvent();
    }
    return true;
  }

  return false;
}

// static
size_t WaitableEvent::WaitMany(WaitableEvent** raw_waitables, size_t count) {
  DCHECK_GT(count, 0u);

  ThreadRestrictions::AssertWaitAllowed();
  debug::ScopedEventWaitActivity event_activity(raw_waitables[0]);

  std::vector<pollfd> pfds(count);
  for (size_t i = 0; i < count; ++i) {
    pfds[i].fd = raw_waitables[i]->event_fd_.get();
    pfds[i].events = POLLIN;
  }

  int rv = HANDLE_EINTR(poll(pfds.data(), count, -1));
  PCHECK(rv >= 0) << "eventfd WaitMany";
  for (size_t i = 0; i < count; ++i) {
    if ((pfds[i].revents & POLLIN) != 0) {
      raw_waitables[i]->ClearEvent();
      return i;
    }
  }

  NOTREACHED();
  return 0;
}

void WaitableEvent::ClearEvent() {
  uint64_t signal = 0;
  ssize_t rv = HANDLE_EINTR(read(event_fd_.get(), &signal, sizeof(signal)));
  if (rv < 0) {
    PCHECK(errno == EAGAIN) << "eventfd read";
  } else {
    DCHECK_EQ(static_cast<size_t>(rv), sizeof(signal));
    // |signal| may be greater than one if multiple calls to Signal() were
    // issued. This is harmless, and it avoids a TOCTOU race for checking
    // IsSignaled() in Signal().
    DCHECK_GE(signal, 1u);
  }
}

}  // namespace base
