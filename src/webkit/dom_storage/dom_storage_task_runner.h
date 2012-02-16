// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef WEBKIT_DOM_STORAGE_DOM_STORAGE_TASK_RUNNER_
#define WEBKIT_DOM_STORAGE_DOM_STORAGE_TASK_RUNNER_
#pragma once

#include "base/memory/ref_counted.h"
#include "base/threading/sequenced_worker_pool.h"

namespace base {
class MessageLoopProxy;
}

namespace dom_storage {

// Tasks must run serially with respect to one another, but may
// execute on different OS threads. The base class is implemented
// in terms of a MessageLoopProxy for use in testing.
class DomStorageTaskRunner
    : public base::RefCountedThreadSafe<DomStorageTaskRunner> {
 public:
  explicit DomStorageTaskRunner(base::MessageLoopProxy* message_loop);
  virtual ~DomStorageTaskRunner();

  // Schedules a task to be run immediately.
  virtual void PostTask(
      const tracked_objects::Location& from_here,
      const base::Closure& task);

  // Schedules a task to be run after a delay.
  virtual void PostDelayedTask(
      const tracked_objects::Location& from_here,
      const base::Closure& task,
      base::TimeDelta delay);

 protected:
  scoped_refptr<base::MessageLoopProxy> message_loop_;
};

// A derived class that utlizes the SequenceWorkerPool under a
// dom_storage specific SequenceToken. The MessageLoopProxy
// is used to delay scheduling on the worker pool.
class DomStorageWorkerPoolTaskRunner : public DomStorageTaskRunner {
 public:
  DomStorageWorkerPoolTaskRunner(
      base::SequencedWorkerPool* sequenced_worker_pool,
      base::MessageLoopProxy* delayed_task_loop);
  virtual ~DomStorageWorkerPoolTaskRunner();

  // Schedules a sequenced worker task to be run immediately.
  virtual void PostTask(
      const tracked_objects::Location& from_here,
      const base::Closure& task) OVERRIDE;

  // Schedules a sequenced worker task to be run after a delay.
  virtual void PostDelayedTask(
      const tracked_objects::Location& from_here,
      const base::Closure& task,
      base::TimeDelta delay) OVERRIDE;

 private:
  base::SequencedWorkerPool* sequenced_worker_pool_;  // not owned
  base::SequencedWorkerPool::SequenceToken sequence_token_;
};

}  // namespace dom_storage

#endif  // WEBKIT_DOM_STORAGE_DOM_STORAGE_TASK_RUNNER_
