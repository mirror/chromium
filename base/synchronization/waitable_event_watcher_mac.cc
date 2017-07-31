// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/synchronization/waitable_event_watcher.h"

#include "base/bind.h"
#include "base/callback.h"
#include "base/threading/sequenced_task_runner_handle.h"

namespace base {

WaitableEventWatcher::WaitableEventWatcher() : weak_ptr_factory_(this) {}

WaitableEventWatcher::~WaitableEventWatcher() {
  StopWatching();
}

bool WaitableEventWatcher::StartWatching(WaitableEvent* event,
                                         EventCallback callback) {
  DCHECK(!source_ || dispatch_source_testcancel(source_));

  // Keep a reference to the receive right, so that if the event is deleted
  // out from under the watcher, a signal can still be observed.
  receive_right_ = event->receive_right_;

  // Use the global concurrent queue here, since it is only used to thunk
  // to the real callback on the target task runner.
  source_.reset(dispatch_source_create(
      DISPATCH_SOURCE_TYPE_MACH_RECV, receive_right_->Name(), 0,
      dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0)));
  callback_ = BindOnce(std::move(callback), event);

  // Locals for capture by the block. Accessing anything through the |this| or
  // |event| pointers is not safe, since either may have been deleted by the
  // time the handler block is invoked.
  scoped_refptr<SequencedTaskRunner> task_runner =
      SequencedTaskRunnerHandle::Get();
  WeakPtr<WaitableEventWatcher> weak_this = weak_ptr_factory_.GetWeakPtr();
  dispatch_source_t source = source_.get();
  mach_port_t name = receive_right_->Name();
  const bool auto_reset = event->auto_reset_;

  dispatch_source_set_event_handler(source_, ^{
    // If another waiter already claimed the signal, leave the dispatch source
    // active to try again.
    if (!WaitableEvent::PeekPort(name, auto_reset)) {
      return;
    }

    // The event has been consumed. A watcher is one-shot, so cancel the
    // source to prevent receiving future event signals.
    dispatch_source_cancel(source);

    task_runner->PostTask(
        FROM_HERE, BindOnce(&WaitableEventWatcher::InvokeCallback, weak_this));
  });
  dispatch_resume(source_);

  return true;
}

void WaitableEventWatcher::StopWatching() {
  callback_.Reset();
  receive_right_ = nullptr;
  if (source_) {
    dispatch_source_cancel(source_);
    source_.reset();
  }
}

void WaitableEventWatcher::InvokeCallback() {
  // The callback can be null if StopWatching() is called between signaling
  // and the |callback_| getting run on the target task runner.
  if (callback_.is_null())
    return;
  source_.reset();
  receive_right_ = nullptr;
  std::move(callback_).Run();
}

}  // namespace base
