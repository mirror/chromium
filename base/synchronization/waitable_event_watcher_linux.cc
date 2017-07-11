// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/synchronization/waitable_event_watcher.h"

#include "base/bind.h"
#include "base/callback.h"
#include "base/task_scheduler/post_task.h"
#include "base/task_scheduler/task_traits.h"
#include "base/threading/sequenced_task_runner_handle.h"
#include "base/threading/thread.h"

namespace base {

class WaitableEventWatcherImpl
    : public RefCountedThreadSafe<WaitableEventWatcherImpl> {
 public:
  WaitableEventWatcherImpl(ScopedFD event_fd, OnceClosure event_callback)
      : task_runner_(CreateSequencedTaskRunnerWithTraits(
            {TaskPriority::HIGHEST, TaskShutdownBehavior::BLOCK_SHUTDOWN})),
        event_fd_(std::move(event_fd)),
        event_task_runner_(SequencedTaskRunnerHandle::Get()),
        event_callback_(std::move(event_callback)) {
    task_runner_->PostTask(
        FROM_HERE, Bind(&WaitableEventWatcherImpl::StartWatching, this));
  }

  void Cancel() {
    DCHECK(event_task_runner_->RunsTasksInCurrentSequence());
    event_callback_.Reset();
    task_runner_->PostTask(FROM_HERE,
                           Bind(&WaitableEventWatcherImpl::StopWatching, this));
  }

 private:
  friend class RefCountedThreadSafe<WaitableEventWatcherImpl>;
  ~WaitableEventWatcherImpl() {}

  // Creates the file descriptor watcher. Must be called on |task_runner_|.
  void StartWatching() {
    DCHECK(task_runner_->RunsTasksInCurrentSequence());
    controller_ = FileDescriptorWatcher::WatchReadable(
        event_fd_.get(), Bind(&WaitableEventWatcherImpl::OnReadReady, this));
    DCHECK(controller_);
  }

  // Stops observing the |event_fd_|. Must be called on |task_runner_|.
  void StopWatching() {
    DCHECK(task_runner_->RunsTasksInCurrentSequence());
    controller_.reset();
    event_fd_.reset();
  }

  // Callback for when the WaitableEvent was signaled. Called on |task_runner_|.
  void OnReadReady() {
    DCHECK(task_runner_->RunsTasksInCurrentSequence());
    event_task_runner_->PostTask(
        FROM_HERE, BindOnce(&WaitableEventWatcherImpl::InvokeCallback, this));
    StopWatching();
  }

  // Invokes the callback on the watcher's |event_task_runner_|. Since
  // cancellation can be occur from a foreign thread, it must be checked before
  // invoking the callback on the target thread.
  void InvokeCallback() {
    DCHECK(event_task_runner_->RunsTasksInCurrentSequence());
    if (!event_callback_.is_null()) {
      std::move(event_callback_).Run();
    }
  }

  // The task runner for the WaitableEventWatcherImpl, which must support
  // FileDescriptorWatcher.
  scoped_refptr<SequencedTaskRunner> task_runner_;

  // The event FD being watched, duplicated from the WaitableEvent's.
  ScopedFD event_fd_;

  // Delivers the OnReadReady() callback when the WaitableEvent is signaled.
  // Will be null if the waiter is cancelled.
  std::unique_ptr<FileDescriptorWatcher::Controller> controller_;

  // Task runner on which |event_callback_| must be invoked.
  scoped_refptr<SequencedTaskRunner> event_task_runner_;

  // The Watcher's event callback.
  OnceClosure event_callback_;

  DISALLOW_COPY_AND_ASSIGN(WaitableEventWatcherImpl);
};

WaitableEventWatcher::WaitableEventWatcher() {}

WaitableEventWatcher::~WaitableEventWatcher() {
  StopWatching();
}

bool WaitableEventWatcher::StartWatching(WaitableEvent* event,
                                         EventCallback callback) {
  ScopedFD event_fd(dup(event->event_fd_.get()));
  if (!event_fd.is_valid()) {
    PLOG(ERROR) << "dup";
    return false;
  }

  OnceClosure bound_callback = BindOnce(std::move(callback), event);
  impl_ = new WaitableEventWatcherImpl(std::move(event_fd),
                                       std::move(bound_callback));

  return true;
}

void WaitableEventWatcher::StopWatching() {
  if (impl_) {
    impl_->Cancel();
    impl_ = nullptr;
  }
}

}  // namespace base
