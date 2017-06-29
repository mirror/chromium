// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/synchronization/waitable_event_watcher.h"

#include "base/bind.h"
#include "base/callback.h"
#include "base/synchronization/waitable_event.h"
#include "base/threading/sequenced_task_runner_handle.h"

namespace base {

WaitableEventWatcher::WaitableEventWatcher() {}

WaitableEventWatcher::~WaitableEventWatcher() {
  StopWatching();
}

bool WaitableEventWatcher::StartWatching(WaitableEvent* event,
                                         EventCallback callback) {
  DCHECK(!source_ || dispatch_source_testcancel(source_));

  kqueue_.reset(dup(event->kqueue_.get()));
  if (!kqueue_.is_valid()) {
    PLOG(ERROR) << "dup kqueue";
    return false;
  }

  source_.reset(dispatch_source_create(DISPATCH_SOURCE_TYPE_READ,
        kqueue_.get(), 0,
        dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0)));

  callback_ = base::BindOnce(std::move(callback), event);

  scoped_refptr<SequencedTaskRunner> task_runner =
      SequencedTaskRunnerHandle::Get();
  dispatch_source_set_event_handler(source_, ^{
    dispatch_source_cancel(source_);

    task_runner->PostTask(FROM_HERE,
        base::BindOnce(&WaitableEventWatcher::InvokeCallback,
                       base::Unretained(this)));
  });
  dispatch_resume(source_);

  return true;
}

void WaitableEventWatcher::StopWatching() {
  callback_.Reset();
  if (source_) {
    dispatch_source_cancel(source_);
    source_.reset();
  }
  kqueue_.reset();
}

void WaitableEventWatcher::InvokeCallback() {
  if (callback_.is_null())
    return;
  source_.reset();
  std::move(callback_).Run();
}

}  // namespace base
