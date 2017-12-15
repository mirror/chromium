// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "cronet_c.h"

#include "base/logging.h"
#include "base/macros.h"
#include "base/test/scoped_task_environment.h"
#include "components/cronet/native/test_util.h"
#include "testing/gtest/include/gtest/gtest.h"

class ExecutorsTest : public ::testing::Test {
 protected:
  ExecutorsTest() = default;
  ~ExecutorsTest() override = default;

 public:
  bool runnable_called_ = false;

  base::test::ScopedTaskEnvironment scoped_task_environment_;

 private:
  DISALLOW_COPY_AND_ASSIGN(ExecutorsTest);
};

namespace {

// App implementation of Cronet_Executor methods.
void TestExecutor_Execute(Cronet_ExecutorPtr self, Cronet_RunnablePtr command) {
  CHECK(self);
  Cronet_Runnable_Run(command);
  Cronet_Runnable_Destroy(command);
}

// Implementation of TestRunnable methods.
void TestRunnable_Run(Cronet_RunnablePtr self) {
  CHECK(self);
  Cronet_RunnableContext context = Cronet_Runnable_GetContext(self);
  ExecutorsTest* test = static_cast<ExecutorsTest*>(context);
  CHECK(test);
  test->runnable_called_ = true;
  DVLOG(1) << "Hello from TestRunnable_Run";
}

}  // namespace

// Test that custom Executor defined by the app runs the runnable.
TEST_F(ExecutorsTest, TestCustom) {
  ASSERT_FALSE(runnable_called_);
  Cronet_RunnablePtr runnable = Cronet_Runnable_CreateStub(TestRunnable_Run);
  Cronet_Runnable_SetContext(runnable, this);
  Cronet_ExecutorPtr executor =
      Cronet_Executor_CreateStub(TestExecutor_Execute);
  Cronet_Executor_Execute(executor, runnable);
  Cronet_Executor_Destroy(executor);
  scoped_task_environment_.RunUntilIdle();
  ASSERT_TRUE(runnable_called_);
}

// Test that cronet::TestUtil::TestExecutor runs the runnable.
TEST_F(ExecutorsTest, TestTestExecutor) {
  ASSERT_FALSE(runnable_called_);
  Cronet_RunnablePtr runnable = Cronet_Runnable_CreateStub(TestRunnable_Run);
  Cronet_Runnable_SetContext(runnable, this);
  Cronet_ExecutorPtr executor = cronet::TestUtil::CreateTestExecutor();
  Cronet_Executor_Execute(executor, runnable);
  Cronet_Executor_Destroy(executor);
  scoped_task_environment_.RunUntilIdle();
  ASSERT_TRUE(runnable_called_);
}
