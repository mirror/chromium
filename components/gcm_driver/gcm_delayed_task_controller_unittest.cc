// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/gcm_driver/gcm_delayed_task_controller.h"

#include <memory>

#include "base/bind.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace gcm {

class GCMDelayedTaskControllerTest : public testing::Test {
 public:
  GCMDelayedTaskControllerTest()
      : controller_(std::make_unique<GCMDelayedTaskController>()) {}
  ~GCMDelayedTaskControllerTest() override = default;

  void TestTask() { ++number_of_triggered_tasks_; }

  bool CanRunTaskWithoutDelay() const {
    return controller_->CanRunTaskWithoutDelayForTesting();
  }

 protected:
  std::unique_ptr<GCMDelayedTaskController> controller_;
  int number_of_triggered_tasks_ = 0;
};

// Tests that a newly created controller forced tasks to be delayed, while
// calling SetReady allows tasks to execute.
TEST_F(GCMDelayedTaskControllerTest, SetReadyWithNoTasks) {
  EXPECT_FALSE(CanRunTaskWithoutDelay());
  EXPECT_EQ(0, number_of_triggered_tasks_);

  controller_->SetReady();
  EXPECT_TRUE(CanRunTaskWithoutDelay());
  EXPECT_EQ(0, number_of_triggered_tasks_);
}

// Tests that tasks are triggered when controller is set to ready.
TEST_F(GCMDelayedTaskControllerTest, PendingTasksTriggeredWhenSetReady) {
  controller_->AddTask(base::BindOnce(&GCMDelayedTaskControllerTest::TestTask,
                                      base::Unretained(this)));
  controller_->AddTask(base::BindOnce(&GCMDelayedTaskControllerTest::TestTask,
                                      base::Unretained(this)));

  controller_->SetReady();
  EXPECT_EQ(2, number_of_triggered_tasks_);
}

// Tests that tasks are triggered immediately when the controller already has
// been set to ready.
TEST_F(GCMDelayedTaskControllerTest, TasksTriggerImmediatelyWhenReady) {
  ASSERT_FALSE(CanRunTaskWithoutDelay());
  EXPECT_EQ(0, number_of_triggered_tasks_);

  controller_->AddTask(base::BindOnce(&GCMDelayedTaskControllerTest::TestTask,
                                      base::Unretained(this)));

  controller_->SetReady();

  ASSERT_TRUE(CanRunTaskWithoutDelay());
  EXPECT_EQ(1, number_of_triggered_tasks_);

  controller_->AddTask(base::BindOnce(&GCMDelayedTaskControllerTest::TestTask,
                                      base::Unretained(this)));

  EXPECT_EQ(2, number_of_triggered_tasks_);
}

}  // namespace gcm
