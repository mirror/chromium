// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/download/internal/task_manager.h"

#include <memory>

#include "base/bind.h"
#include "components/download/public/task_scheduler.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

using testing::_;

namespace download {
namespace {

class MockTaskScheduler : public TaskScheduler {
 public:
  MockTaskScheduler() = default;
  ~MockTaskScheduler() override = default;

  // TaskScheduler implementation.
  MOCK_METHOD5(ScheduleTask, void(DownloadTaskType, bool, bool, long, long));
  MOCK_METHOD1(CancelTask, void(DownloadTaskType));
};

class MockTaskWaiter {
 public:
  MockTaskWaiter() = default;
  virtual ~MockTaskWaiter() = default;

  MOCK_METHOD1(TaskFinished, void(bool));
};

TEST(DownloadServiceTaskManagerTest, TestSchedule) {
  std::unique_ptr<TaskScheduler> scheduler =
      std::make_unique<MockTaskScheduler>();
  MockTaskScheduler* raw_scheduler =
      static_cast<MockTaskScheduler*>(scheduler.get());
  TaskManager manager(std::move(scheduler));

  TaskManager::TaskParams params;
  params.requires_unmetered_network = false;
  params.requires_battery_charging = false;
  params.window_start_seconds = 10;
  params.window_end_seconds = 20;

  EXPECT_CALL(*raw_scheduler, ScheduleTask(DownloadTaskType::DOWNLOAD_TASK,
                                           params.requires_unmetered_network,
                                           params.requires_battery_charging,
                                           params.window_start_seconds,
                                           params.window_end_seconds))
      .Times(1);
  manager.ScheduleTask(DownloadTaskType::DOWNLOAD_TASK, params);

  params.requires_unmetered_network = true;
  EXPECT_CALL(*raw_scheduler, ScheduleTask(DownloadTaskType::DOWNLOAD_TASK,
                                           params.requires_unmetered_network,
                                           params.requires_battery_charging,
                                           params.window_start_seconds,
                                           params.window_end_seconds))
      .Times(1);
  manager.ScheduleTask(DownloadTaskType::DOWNLOAD_TASK, params);

  EXPECT_CALL(*raw_scheduler, ScheduleTask(_, _, _, _, _)).Times(0);
  manager.ScheduleTask(DownloadTaskType::DOWNLOAD_TASK, params);
}

TEST(DownloadServiceTaskManagerTest, TestUnschedule) {
  std::unique_ptr<TaskScheduler> scheduler =
      std::make_unique<MockTaskScheduler>();
  MockTaskScheduler* raw_scheduler =
      static_cast<MockTaskScheduler*>(scheduler.get());
  TaskManager manager(std::move(scheduler));

  TaskManager::TaskParams params;
  params.requires_unmetered_network = false;
  params.requires_battery_charging = false;
  params.window_start_seconds = 10;
  params.window_end_seconds = 20;

  EXPECT_CALL(*raw_scheduler, CancelTask(DownloadTaskType::DOWNLOAD_TASK))
      .Times(1);
  manager.UnscheduleTask(DownloadTaskType::DOWNLOAD_TASK);
  manager.UnscheduleTask(DownloadTaskType::DOWNLOAD_TASK);

  manager.ScheduleTask(DownloadTaskType::DOWNLOAD_TASK, params);

  EXPECT_CALL(*raw_scheduler, CancelTask(DownloadTaskType::DOWNLOAD_TASK))
      .Times(1);
  manager.UnscheduleTask(DownloadTaskType::DOWNLOAD_TASK);

  EXPECT_CALL(*raw_scheduler, CancelTask(_)).Times(0);
  manager.UnscheduleTask(DownloadTaskType::DOWNLOAD_TASK);
}

TEST(DownloadServiceTaskManagerTest, TestRunningTask) {
  std::unique_ptr<TaskScheduler> scheduler =
      std::make_unique<MockTaskScheduler>();
  MockTaskScheduler* raw_scheduler =
      static_cast<MockTaskScheduler*>(scheduler.get());
  TaskManager manager(std::move(scheduler));

  MockTaskWaiter waiter1;
  MockTaskWaiter waiter2;

  auto callback1 =
      base::Bind(&MockTaskWaiter::TaskFinished, base::Unretained(&waiter1));
  auto callback2 =
      base::Bind(&MockTaskWaiter::TaskFinished, base::Unretained(&waiter2));

  EXPECT_CALL(waiter1, TaskFinished(_)).Times(0);
  EXPECT_CALL(waiter2, TaskFinished(_)).Times(0);
  manager.OnSystemStartedTask(DownloadTaskType::DOWNLOAD_TASK, callback1);
  manager.OnSystemStartedTask(DownloadTaskType::CLEANUP_TASK, callback2);

  EXPECT_CALL(*raw_scheduler, ScheduleTask(_, _, _, _, _)).Times(0);
  EXPECT_CALL(*raw_scheduler, CancelTask(_)).Times(0);

  TaskManager::TaskParams params;
  params.requires_unmetered_network = false;
  params.requires_battery_charging = false;
  params.window_start_seconds = 10;
  params.window_end_seconds = 20;

  manager.ScheduleTask(DownloadTaskType::DOWNLOAD_TASK, params);
  manager.ScheduleTask(DownloadTaskType::CLEANUP_TASK, params);
  manager.UnscheduleTask(DownloadTaskType::CLEANUP_TASK);

  EXPECT_CALL(*raw_scheduler, ScheduleTask(DownloadTaskType::DOWNLOAD_TASK,
                                           params.requires_unmetered_network,
                                           params.requires_battery_charging,
                                           params.window_start_seconds,
                                           params.window_end_seconds))
      .Times(1);
  EXPECT_CALL(waiter1, TaskFinished(false)).Times(1);

  // Ignore reschedule because we have new params ready to go.
  manager.FinishTask(DownloadTaskType::DOWNLOAD_TASK, true,
                     stats::ScheduledTaskStatus::COMPLETED_NORMALLY);
  manager.FinishTask(DownloadTaskType::DOWNLOAD_TASK, false,
                     stats::ScheduledTaskStatus::COMPLETED_NORMALLY);

  EXPECT_CALL(waiter2, TaskFinished(false)).Times(1);
  EXPECT_CALL(*raw_scheduler, CancelTask(DownloadTaskType::CLEANUP_TASK));
  manager.FinishTask(DownloadTaskType::CLEANUP_TASK, false,
                     stats::ScheduledTaskStatus::COMPLETED_NORMALLY);
}

}  // namespace
}  // namespace download
