// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/synchronization/waitable_event.h"

#include <mach/mach.h>

#include "base/debug/activity_tracker.h"
#include "base/mac/mach_logging.h"
#include "base/threading/thread_restrictions.h"

namespace base {

WaitableEvent::WaitableEvent(ResetPolicy reset_policy,
                             InitialState initial_state)
    : auto_reset_(reset_policy == ResetPolicy::AUTOMATIC) {
  mach_port_options_t options{};
  options.flags = MPO_INSERT_SEND_RIGHT;
  options.mpl.mpl_qlimit = 1;

  mach_port_t name;
  kern_return_t kr = mach_port_construct(
      mach_task_self(), &options, reinterpret_cast<uint64_t>(this), &name);
  MACH_CHECK(kr == KERN_SUCCESS, kr) << "mach_port_construct";

  receive_right_ = new ReceiveRight(name);
  send_right_.reset(name);

  if (initial_state == InitialState::SIGNALED)
    Signal();
}

WaitableEvent::~WaitableEvent() = default;

void WaitableEvent::Reset() {
  PeekPort(receive_right_->Name(), true);
}

void WaitableEvent::Signal() {
  mach_msg_empty_send_t msg{};
  msg.header.msgh_bits = MACH_MSGH_BITS_REMOTE(MACH_MSG_TYPE_COPY_SEND);
  msg.header.msgh_size = sizeof(&msg);
  msg.header.msgh_remote_port = send_right_.get();
  // If the event is already signaled, this will time out because the queue
  // has a length of one.
  kern_return_t kr =
      mach_msg(&msg.header, MACH_SEND_MSG | MACH_SEND_TIMEOUT, sizeof(msg), 0,
               MACH_PORT_NULL, 0, MACH_PORT_NULL);
  MACH_CHECK(kr == KERN_SUCCESS || kr == MACH_SEND_TIMED_OUT, kr) << "mach_msg";
}

bool WaitableEvent::IsSignaled() {
  return PeekPort(receive_right_->Name(), auto_reset_);
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
  // Record the event that this thread is blocking upon (for hang diagnosis).
  debug::ScopedEventWaitActivity event_activity(this);

  TimeDelta wait_time = end_time - TimeTicks::Now();
  if (wait_time < TimeDelta()) {
    // A negative delta would be treated by the system as indefinite, but
    // it needs to be treated as a poll instead.
    wait_time = TimeDelta();
  }

  mach_msg_empty_rcv_t msg{};
  msg.header.msgh_local_port = receive_right_->Name();

  mach_msg_option_t options = MACH_RCV_MSG;

  mach_msg_timeout_t timeout = 0;
  if (!end_time.is_max()) {
    options |= MACH_RCV_TIMEOUT;
    timeout = wait_time.InMillisecondsRoundedUp();
  }

  mach_msg_size_t rcv_size = sizeof(msg);
  if (!auto_reset_) {
    // To avoid dequeing the message for manual-reset events, receive with a
    // size of 0 and set MACH_RCV_LARGE to keep the message in the queue.
    options |= MACH_RCV_LARGE;
    rcv_size = 0;
  }

  kern_return_t kr = mach_msg(&msg.header, options, 0, rcv_size,
                              receive_right_->Name(), timeout, MACH_PORT_NULL);
  if (kr == KERN_SUCCESS) {
    return true;
  } else if (rcv_size == 0 && kr == MACH_RCV_TOO_LARGE) {
    return true;
  } else {
    MACH_CHECK(kr == MACH_RCV_TIMED_OUT, kr) << "mach_msg";
    return false;
  }
}

// static
size_t WaitableEvent::WaitMany(WaitableEvent** raw_waitables, size_t count) {
  ThreadRestrictions::AssertWaitAllowed();
  DCHECK(count) << "Cannot wait on no events";

  kern_return_t kr;

  mac::ScopedMachPortSet port_set;
  {
    mach_port_t name;
    kr = mach_port_allocate(mach_task_self(), MACH_PORT_RIGHT_PORT_SET, &name);
    MACH_CHECK(kr == KERN_SUCCESS, kr) << "mach_port_allocate";
    port_set.reset(name);
  }

  for (size_t i = 0; i < count; ++i) {
    kr = mach_port_insert_member(mach_task_self(),
                                 raw_waitables[i]->receive_right_->Name(),
                                 port_set.get());
    MACH_CHECK(kr == KERN_SUCCESS, kr) << "index " << i;
  }

  mach_msg_empty_rcv_t msg{};
  // Wait on the port set. Only specify space enough for the header, to
  // identify which port in the set is signaled. Otherwise, receiving from the
  // port set may dequeue a message for a manual-reset event object, which
  // would cause it to be reset.
  kr = mach_msg(&msg.header,
                MACH_RCV_MSG | MACH_RCV_LARGE | MACH_RCV_LARGE_IDENTITY, 0,
                sizeof(msg.header), port_set.get(), 0, MACH_PORT_NULL);
  MACH_CHECK(kr == MACH_RCV_TOO_LARGE, kr) << "mach_msg";

  for (size_t i = 0; i < count; ++i) {
    WaitableEvent* event = raw_waitables[i];
    if (msg.header.msgh_local_port == event->receive_right_->Name()) {
      if (event->auto_reset_) {
        // For auto-reset events, the message needs to be dequeued to reset the
        // event.
        PeekPort(msg.header.msgh_local_port, true);
      }
      return i;
    }
  }

  NOTREACHED();
  return 0;
}

// static
bool WaitableEvent::PeekPort(mach_port_t port, bool dequeue) {
  if (dequeue) {
    mach_msg_empty_rcv_t msg{};
    msg.header.msgh_local_port = port;
    kern_return_t kr = mach_msg(&msg.header, MACH_RCV_MSG | MACH_RCV_TIMEOUT, 0,
                                sizeof(msg), port, 0, MACH_PORT_NULL);
    if (kr == KERN_SUCCESS) {
      return true;
    } else {
      MACH_CHECK(kr == MACH_RCV_TIMED_OUT, kr) << "mach_msg";
      return false;
    }
  } else {
    mach_port_seqno_t seqno = 0;
    mach_msg_size_t size;
    mach_msg_id_t id;
    mach_msg_trailer_t trailer;
    mach_msg_type_number_t trailer_size = sizeof(trailer);
    kern_return_t kr = mach_port_peek(
        mach_task_self(), port, MACH_RCV_TRAILER_TYPE(MACH_RCV_TRAILER_NULL),
        &seqno, &size, &id, reinterpret_cast<mach_msg_trailer_info_t>(&trailer),
        &trailer_size);
    if (kr == KERN_SUCCESS) {
      return true;
    } else {
      MACH_CHECK(kr == KERN_FAILURE, kr) << "mach_port_peek";
      return false;
    }
  }
}

WaitableEvent::ReceiveRight::ReceiveRight(mach_port_t name) : right_(name) {}

WaitableEvent::ReceiveRight::~ReceiveRight() = default;

}  // namespace base
