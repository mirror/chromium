// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef GIN_V8_TASKRUNNER_H
#define GIN_V8_TASKRUNNER_H

#include "base/memory/ref_counted.h"
#include "gin/public/isolate_holder.h"
#include "v8/include/v8-platform.h"

namespace base {
class SingleThreadTaskRunner;
}

namespace gin {

class V8ForegroundTaskRunner : public v8::TaskRunner {
 public:
  V8ForegroundTaskRunner(
      v8::Isolate* isolate,
      IsolateHolder::AccessMode access_mode,
      scoped_refptr<base::SingleThreadTaskRunner> task_runner);

  ~V8ForegroundTaskRunner() override;

  void PostTask(std::unique_ptr<v8::Task> task) override;

  void PostDelayedTask(std::unique_ptr<v8::Task> task,
                       double delay_in_seconds) override;

  void PostIdleTask(std::unique_ptr<v8::IdleTask> task) override;

  bool IdleTasksEnabled() override;

  void EnableIdleTasks(std::unique_ptr<V8IdleTaskRunner> idle_task_runner);

 private:
  v8::Isolate* isolate_;
  IsolateHolder::AccessMode access_mode_;
  scoped_refptr<base::SingleThreadTaskRunner> task_runner_;
  std::unique_ptr<V8IdleTaskRunner> idle_task_runner_;
};

class V8BackgroundTaskRunner : public v8::TaskRunner {
 public:
  void PostTask(std::unique_ptr<v8::Task> task) override;

  void PostDelayedTask(std::unique_ptr<v8::Task> task,
                       double delay_in_seconds) override;

  void PostIdleTask(std::unique_ptr<v8::IdleTask> task) override;

  bool IdleTasksEnabled() override;

  static size_t NumberOfAvailableBackgroundThreads();
};

}  // namespace gin

#endif  // V8_TASKRUNNER_H
