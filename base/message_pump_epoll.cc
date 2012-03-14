// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/message_pump_epoll.h"

#include <math.h>
#include <sys/epoll.h>

#include "base/eintr_wrapper.h"

namespace base {

namespace {

MessagePumpDispatcher* g_default_dispatcher = NULL;

// Return a timeout suitable for the epoll loop, -1 to block forever,
// 0 to return right away, or a timeout in milliseconds from now.
int GetTimeIntervalMilliseconds(const base::TimeTicks& from) {
  if (from.is_null())
    return -1;

  // Be careful here.  TimeDelta has a precision of microseconds, but we want a
  // value in milliseconds.  If there are 5.5ms left, should the delay be 5 or
  // 6?  It should be 6 to avoid executing delayed work too early.
  int delay = static_cast<int>(
      ceil((from - TimeTicks::Now()).InMillisecondsF()));

  // If this value is negative, then we need to run delayed work soon.
  return delay < 0 ? 0 : delay;
}

}  // namespace

struct MessagePumpEpoll::RunState {
  MessagePump::Delegate* delegate;
  MessagePumpDispatcher* dispatcher;

  // Used to flag that the current Run() invocation should return ASAP.
  bool should_quit;

  // Used to count how many Run() invocations are on the stack.
  int run_depth;

  // This keeps the state of whether the pump got signaled that there was new
  // work to be done.
  bool has_work;
};

MessagePumpEpoll::MessagePumpEpoll()
    : state_(NULL), 
      num_fds_(0),
      wakeup_handler_(new MessagePumpEpollHandler(this)) {
  int fds[2];
  // Create our wakeup pipe, which is used to flag when work was scheduled.
  int ret = pipe(fds);
  DCHECK_EQ(ret, 0);
  (void)ret;  // Prevent warning in release mode.

  epoll_fd_ = epoll_create1(EPOLL_CLOEXEC);
  DCHECK_GE(epoll_fd_, 0);

  wakeup_pipe_write_ = fds[1];
  wakeup_handler_->set_fd(fds[0]);
  AddHandler(wakeup_handler_);
}

MessagePumpEpoll::~MessagePumpEpoll() {
  close(wakeup_pipe_write_);
  delete wakeup_handler_;
}

void MessagePumpEpoll::Run(MessagePump::Delegate* delegate) {
  RunWithDispatcher(delegate, NULL);
}

void MessagePumpEpoll::RunWithDispatcher(MessagePump::Delegate* delegate,
                                       MessagePumpDispatcher* dispatcher) {
  RunState state;
  state.delegate = delegate;
  state.dispatcher = dispatcher;
  state.should_quit = false;
  state.run_depth = state_ ? state_->run_depth + 1 : 1;
  state.has_work = false;

  RunState* previous_state = state_;
  state_ = &state;

  // We really only do a single task for each iteration of the loop.  If we
  // have done something, assume there is likely something more to do.  This
  // will mean that we don't block on the message pump until there was nothing
  // more to do.

  bool more_work_is_plausible = false;
  for (;;) {
    // Don't block if we think we have more work to do.
    bool block = !more_work_is_plausible;

    more_work_is_plausible = DoInternalWork(block);
    if (state_->should_quit)
      break;

    more_work_is_plausible |= state_->delegate->DoWork();
    if (state_->should_quit)
      break;

    more_work_is_plausible |=
        state_->delegate->DoDelayedWork(&delayed_work_time_);
    if (state_->should_quit)
      break;

    if (more_work_is_plausible)
      continue;

    more_work_is_plausible = state_->delegate->DoIdleWork();
    if (state_->should_quit)
      break;
  }

  state_ = previous_state;
}

void MessagePumpEpoll::AddHandler(MessagePumpEpollHandler* handler) {
  struct epoll_event ep = {};
  int ret;

  ep.events = EPOLLIN;
  ep.data.ptr = handler;

  ret = epoll_ctl(epoll_fd_, EPOLL_CTL_ADD, handler->fd(), &ep);
  DCHECK_EQ(ret, 0);
  (void)ret;  // Prevent warning in release mode.

  num_fds_++;
}

void MessagePumpEpoll::RemoveHandler(MessagePumpEpollHandler* handler) {
  int ret;

  ret = epoll_ctl(epoll_fd_, EPOLL_CTL_DEL, handler->fd(), NULL);
  DCHECK_EQ(ret, 0);
  (void)ret;  // Prevent warning in release mode.

  num_fds_--;
}

void MessagePumpEpoll::AddObserver(MessagePumpObserver* observer) {
  observers_.AddObserver(observer);
}

void MessagePumpEpoll::RemoveObserver(MessagePumpObserver* observer) {
  observers_.RemoveObserver(observer);
}

void MessagePumpEpoll::Quit() {
  if (state_)
    state_->should_quit = true;
  else
    NOTREACHED() << "Quit called outside Run!";
}

void MessagePumpEpoll::ScheduleWork() {
  // This can be called on any thread, so we don't want to touch any state
  // variables as we would then need locks all over.  This ensures that if
  // we are sleeping in a poll that we will wake up.
  if (HANDLE_EINTR(write(wakeup_pipe_write_, "", 1)) != 1)
    NOTREACHED() << "Could not write to the UI message loop wakeup pipe!";
}

void MessagePumpEpoll::ScheduleDelayedWork(const TimeTicks& delayed_work_time) {
  // We need to wake up the loop in case the poll timeout needs to be
  // adjusted.  This will cause us to try to do work, but that's ok.
  delayed_work_time_ = delayed_work_time;
  ScheduleWork();
}

// static
void MessagePumpEpoll::SetDefaultDispatcher(MessagePumpDispatcher* dispatcher) {
  DCHECK(!g_default_dispatcher || !dispatcher);
  g_default_dispatcher = dispatcher;
}

bool MessagePumpEpoll::DoInternalWork(bool block) {
  struct epoll_event events[num_fds_];

  int timeout = 0;
  if (block)
    timeout = GetTimeIntervalMilliseconds(delayed_work_time_);

  int nfds = epoll_wait(epoll_fd_, events, num_fds_, timeout);
  for (int i = 0; i < nfds; ++i)
    static_cast<MessagePumpEpollHandler*>(events[i].data.ptr)->Process();

  return nfds > 0;
}

bool MessagePumpEpoll::DispatchEvent(NativeEvent event) {
  if (WillProcessEvent(event))
    return false;
  MessagePumpDispatcher* dispatcher = GetDispatcher();
  if (!dispatcher)
    return false;
  MessagePumpDispatcher::DispatchStatus status = dispatcher->Dispatch(event);
  if (status == MessagePumpDispatcher::EVENT_QUIT)
    Quit();
  else if (status == MessagePumpDispatcher::EVENT_IGNORED)
    VLOG(1) << "Event not handled.";
  DidProcessEvent(event);
  return true;
}

bool MessagePumpEpoll::WillProcessEvent(NativeEvent event) {
  if (!observers_.might_have_observers())
    return false;
  ObserverListBase<MessagePumpObserver>::Iterator it(observers_);
  MessagePumpObserver* obs;
  while ((obs = it.GetNext()) != NULL)
    if (obs->WillProcessEvent(event))
      return true;
  return false;
}

void MessagePumpEpoll::DidProcessEvent(NativeEvent event) {
  FOR_EACH_OBSERVER(MessagePumpObserver, observers_, DidProcessEvent(event));
}

MessagePumpDispatcher* MessagePumpEpoll::GetDispatcher() const {
  return (state_ && state_->dispatcher) ? state_->dispatcher :
      g_default_dispatcher;
}

}  // namespace base
