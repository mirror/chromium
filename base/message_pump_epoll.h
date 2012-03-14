// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef BASE_MESSAGE_PUMP_EPOLL_H
#define BASE_MESSAGE_PUMP_EPOLL_H

#if !defined(MESSAGE_PUMP_FOR_UI)
#define MESSAGE_PUMP_EPOLL_FOR_UI
#define MESSAGE_PUMP_FOR_UI
#endif

#include "base/message_pump.h"

#include "base/message_pump_observer.h"
#include "base/observer_list.h"
#include "base/time.h"

namespace base {

class MessagePumpEpoll;

// MessagePumpDispatcher is used during a nested invocation of Run to dispatch
// events. If Run is invoked with a non-NULL MessagePumpDispatcher, MessageLoop
// does not dispatch events, rather every event is passed to Dispatcher's
// Dispatch method for dispatch. It is up to the Dispatcher to dispatch, or not,
// the event.
// The nested loop is exited by either posting a quit, or returning EVENT_QUIT
// from Dispatch.
class MessagePumpDispatcher {
 public:
  enum DispatchStatus {
    EVENT_IGNORED,    // The event was not processed.
    EVENT_PROCESSED,  // The event has been processed.
    EVENT_QUIT        // The event was processed and the message-loop should
    // terminate.
  };

  // Dispatches the event. EVENT_IGNORED is returned if the event was ignored
  // (i.e. not processed). EVENT_PROCESSED is returned if the event was
  // processed. The nested loop exits immediately if EVENT_QUIT is returned.
  virtual DispatchStatus Dispatch(NativeEvent event) = 0;

 protected:
  virtual ~MessagePumpDispatcher() {}
};

// MessagePumpEpollHandler is use to register an fd to be handled by the
// event loop. The Process function will be called when the fd is ready.
class MessagePumpEpollHandler {
 public:
  MessagePumpEpollHandler(MessagePumpEpoll *pump) : fd_(-1) { pump_ = pump; };
  virtual ~MessagePumpEpollHandler() { if (fd_ >= 0) close(fd_); };

  // Event handler for an fd registered with MessagePumpEpoll.
  // Responsible for dispatching events via pump->Dispatch().
  virtual void Process() { };

  int fd() { return fd_; };
  void set_fd(int fd) { fd_ = fd; };

 protected:
  MessagePumpEpoll *pump_;
  int fd_;

  DISALLOW_COPY_AND_ASSIGN(MessagePumpEpollHandler);
};

// This class implements a message-pump for processing events from input devices
// Refer to MessagePump for further documentation
class BASE_EXPORT MessagePumpEpoll
    : public MessagePump {
 public:
  MessagePumpEpoll();
  virtual ~MessagePumpEpoll();

  virtual void Run(MessagePump::Delegate* delegate) OVERRIDE;
  // Like MessagePump::Run, but events are routed through dispatcher.
  void RunWithDispatcher(MessagePump::Delegate* delegate,
                         MessagePumpDispatcher* dispatcher);

  // Adds an Observer, which will start receiving notifications immediately.
  void AddObserver(MessagePumpObserver* observer);

  // Removes an Observer.  It is safe to call this method while an Observer is
  // receiving a notification callback.
  void RemoveObserver(MessagePumpObserver* observer);

  // Overridden from MessagePump:
  virtual void Quit() OVERRIDE;
  virtual void ScheduleWork() OVERRIDE;
  virtual void ScheduleDelayedWork(const TimeTicks& delayed_work_time) OVERRIDE;

  // Sets the default dispatcher to process native events.
  static void SetDefaultDispatcher(MessagePumpDispatcher* dispatcher);

  // Calls WillProcessEvent, and then dispatches event and calls DidProcessEvent
  // as necessary. Returns true if event was dispatched.
  bool DispatchEvent(NativeEvent event);

  // Adds an fd Handler, which will handle events on an fd.
  void AddHandler(MessagePumpEpollHandler* handler);

  // Removes a Handler.
  void RemoveHandler(MessagePumpEpollHandler* handler);

 private:
  // We may make recursive calls to Run, so we save state that needs to be
  // separate between them in this structure type.
  struct RunState;

  RunState* state_;

  // epoll event loop. Returns true if work was done.
  bool DoInternalWork(bool block);

  // Sends the event to the observers. If an observer returns true, then it does
  // not send the event to any other observers and returns true. Returns false
  // if no observer returns true.
  bool WillProcessEvent(NativeEvent event);
  void DidProcessEvent(NativeEvent event);

  // Returns the dispatcher for the current run state (|state_->dispatcher|).
  MessagePumpDispatcher* GetDispatcher() const;

  // This is the time when we need to do delayed work.
  TimeTicks delayed_work_time_;

  int epoll_fd_;
  int num_fds_;
  
  // We use a wakeup pipe to make sure we'll get out of the polling phase
  // when another thread has scheduled us to do some work.
  int wakeup_pipe_write_;
  MessagePumpEpollHandler *wakeup_handler_;

  // List of observers.
  ObserverList<MessagePumpObserver> observers_;

  DISALLOW_COPY_AND_ASSIGN(MessagePumpEpoll);
};

#if defined(MESSAGE_PUMP_EPOLL_FOR_UI)
typedef MessagePumpEpoll MessagePumpForUI;
#endif

}  // namespace base

#endif  // BASE_MESSAGE_PUMP_EPOLL_H
