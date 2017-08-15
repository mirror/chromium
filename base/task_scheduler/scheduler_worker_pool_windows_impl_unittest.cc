// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/task_scheduler/scheduler_worker_pool_windows_impl.h"

#include "base/bind.h"
#include "base/bind_helpers.h"
#include "base/location.h"
#include "base/memory/ptr_util.h"
#include "base/memory/ref_counted.h"
#include "base/task_scheduler/delayed_task_manager.h"
#include "base/task_scheduler/task_tracker.h"
#include "base/task_scheduler/task_traits.h"
#include "base/test/test_timeouts.h"
#include "base/threading/platform_thread.h"
#include "base/threading/thread.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace base {
namespace internal {

namespace {

class TaskSchedulerWorkerPoolWindowsImplTest : public testing::Test {
 protected:
  TaskSchedulerWorkerPoolWindowsImplTest()
      : service_thread_("TaskSchedulerServiceThread") {}

  void SetUp() override {
    service_thread_.Start();
    delayed_task_manager_.Start(service_thread_.task_runner());
    worker_pool_ = MakeUnique<SchedulerWorkerPoolWindowsImpl>(
        &task_tracker_, &delayed_task_manager_);
  }

  void TearDown() override { service_thread_.Stop(); }

  std::unique_ptr<SchedulerWorkerPoolWindowsImpl> worker_pool_;

  TaskTracker task_tracker_;

 private:
  Thread service_thread_;
  DelayedTaskManager delayed_task_manager_;

  DISALLOW_COPY_AND_ASSIGN(TaskSchedulerWorkerPoolWindowsImplTest);
};

}  // namespace

void SignalEvent(WaitableEvent* event) {
  event->Signal();
}

// Verify that after tasks posted before Start run after Start.
TEST_F(TaskSchedulerWorkerPoolWindowsImplTest, PostBeforeStart) {
  WaitableEvent task_1_scheduled(WaitableEvent::ResetPolicy::MANUAL,
                                 WaitableEvent::InitialState::NOT_SIGNALED);
  WaitableEvent task_2_scheduled(WaitableEvent::ResetPolicy::MANUAL,
                                 WaitableEvent::InitialState::NOT_SIGNALED);

  scoped_refptr<TaskRunner> task_runner =
      worker_pool_->CreateTaskRunnerWithTraits({WithBaseSyncPrimitives()});

  task_runner->PostTask(FROM_HERE,
                        BindOnce(&SignalEvent, Unretained(&task_1_scheduled)));
  task_runner->PostTask(FROM_HERE,
                        BindOnce(&SignalEvent, Unretained(&task_2_scheduled)));

  // Workers should not be created and tasks should not run before the pool is
  // started. The sleep is to give time for the tasks to potentially run.
  PlatformThread::Sleep(TestTimeouts::tiny_timeout());
  EXPECT_FALSE(task_1_scheduled.IsSignaled());
  EXPECT_FALSE(task_2_scheduled.IsSignaled());

  worker_pool_->Start();

  // Tasks should be scheduled shortly after the pool is started.
  task_1_scheduled.Wait();
  task_2_scheduled.Wait();

  task_tracker_.Flush();
}

}  // namespace internal
}  // namespace base
