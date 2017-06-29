// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/synchronization/waitable_event_watcher.h"

#include "base/bind.h"
#include "base/callback.h"
#include "base/synchronization/waitable_event.h"
#include "base/threading/sequenced_task_runner_handle.h"

namespace base {

namespace {

void DispatchWaitableEventCallback(
    ScopedDispatchObject<dispatch_source_t> source,
    OnceClosure&& callback) {
  if (dispatch_source_testcancel(source))
    return;
  std::move(callback).Run();
  dispatch_source_cancel(source);
}

}  // namespace

WaitableEventWatcher::WaitableEventWatcher() {}

WaitableEventWatcher::~WaitableEventWatcher() {
  StopWatching();
}

bool WaitableEventWatcher::StartWatching(WaitableEvent* event,
                                         EventCallback callback) {
  DCHECK(!source_ || dispatch_source_testcancel(source_));

  source_.reset(dispatch_source_create(DISPATCH_SOURCE_TYPE_DATA_ADD,
        0, 0, event->dispatch_queue_));

  scoped_refptr<SequencedTaskRunner> task_runner =
      SequencedTaskRunnerHandle::Get();
  OnceClosure __block bound_callback =
      base::BindOnce(std::move(callback), event);

  dispatch_source_set_event_handler(source_, ^{
    task_runner->PostTask(FROM_HERE,
        base::BindOnce(&DispatchWaitableEventCallback,
                       source_, std::move(bound_callback)));
  });
  dispatch_resume(source_);

  event->AddWatcherSource(source_);

  return true;
}

void WaitableEventWatcher::StopWatching() {
  if (source_) {
    dispatch_source_cancel(source_);
    source_.reset();
  }
}

}  // namespace base
