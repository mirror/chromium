// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/cronet/native/executors.h"

#include "base/logging.h"
#include "base/macros.h"
#include "base/test/scoped_task_environment.h"
#include "testing/gtest/include/gtest/gtest.h"

class ExecutorsTest : public ::testing::Test {
 protected:
  void SetUp() override {}

  void TearDown() override {}

  ExecutorsTest() = default;
  ~ExecutorsTest() override {}

 public:
  bool runnable_called_ = false;

  base::test::ScopedTaskEnvironment scoped_task_environment_;

 private:
  DISALLOW_COPY_AND_ASSIGN(ExecutorsTest);
};

// Implementation of TestRunnable methods.
void TestRunnable_Run(Cronet_RunnablePtr self) {
  CHECK(self);
  Cronet_RunnableContext context = Cronet_Runnable_GetContext(self);
  ExecutorsTest* test = static_cast<ExecutorsTest*>(context);
  CHECK(test);
  test->runnable_called_ = true;
  DVLOG(1) << "Hello from TestRunnable_Run";
}

// Test that DirectExecutor runs the runnable.
TEST_F(ExecutorsTest, TestDirect) {
  ASSERT_FALSE(runnable_called_);
  Cronet_RunnablePtr runnable = Cronet_Runnable_CreateStub(TestRunnable_Run);
  Cronet_Runnable_SetContext(runnable, this);
  Cronet_ExecutorPtr executor = DirectExecutor_Create();
  Cronet_Executor_Execute(executor, runnable);
  Cronet_Executor_Destroy(executor);
  ASSERT_TRUE(runnable_called_);
}

// Test that SequencedExecutor runs the runnable.
TEST_F(ExecutorsTest, TestTaskRunnerExecutor) {
  ASSERT_FALSE(runnable_called_);
  Cronet_RunnablePtr runnable = Cronet_Runnable_CreateStub(TestRunnable_Run);
  Cronet_Runnable_SetContext(runnable, this);
  Cronet_ExecutorPtr executor = TaskRunnerExecutor_Create();
  Cronet_Executor_Execute(executor, runnable);
  Cronet_Executor_Destroy(executor);
  scoped_task_environment_.RunUntilIdle();
  ASSERT_TRUE(runnable_called_);
}
