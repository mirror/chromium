// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/cronet/native/generated/cronet.idl_c.h"

#include "base/logging.h"
#include "base/macros.h"
#include "base/test/scoped_task_environment.h"

#include "testing/gtest/include/gtest/gtest.h"

// TODO(mef): Move to executors.cc
#include "base/task_scheduler/post_task.h"
#include "base/task_scheduler/task_traits.h"

namespace {

// Implementation of DirectExecutor methods.
void DirectExecutor_Execute(Cronet_ExecutorPtr self,
                            Cronet_RunnablePtr command) {
  CHECK(self);
  DLOG(INFO) << "Running directly";
  Cronet_Runnable_Run(command);
  DLOG(INFO) << "Destroying command";
  Cronet_Runnable_Destroy(command);
}

Cronet_ExecutorPtr DirectExecutor_Create() {
  return Cronet_Executor_CreateStub(DirectExecutor_Execute);
}

// Implementation of PostTaskExecutor methods.
void PostTaskExecutor_Execute(Cronet_ExecutorPtr self,
                              Cronet_RunnablePtr command) {
  CHECK(self);
  DLOG(INFO) << "Post Task";
  base::PostTask(FROM_HERE, base::BindOnce(
                                [](Cronet_RunnablePtr command) {
                                  DLOG(INFO) << "Running posted task";
                                  Cronet_Runnable_Run(command);
                                  DLOG(INFO) << "Destroying command";
                                  Cronet_Runnable_Destroy(command);
                                },
                                command));
}

Cronet_ExecutorPtr PostTaskExecutor_Create() {
  return Cronet_Executor_CreateStub(PostTaskExecutor_Execute);
}

class SequencedExecutor {
 public:
  static Cronet_ExecutorPtr Create();

 private:
  SequencedExecutor();
  static void Execute(Cronet_ExecutorPtr self, Cronet_RunnablePtr command);

  scoped_refptr<base::SequencedTaskRunner> task_runner_;
  DISALLOW_COPY_AND_ASSIGN(SequencedExecutor);
};

SequencedExecutor::SequencedExecutor()
    : task_runner_(CreateSequencedTaskRunnerWithTraits(base::TaskTraits())) {}

// static
Cronet_ExecutorPtr SequencedExecutor::Create() {
  Cronet_ExecutorPtr executor = Cronet_Executor_CreateStub(Execute);
  Cronet_Executor_SetContext(executor, new SequencedExecutor());
  return executor;
}

// static
void SequencedExecutor::Execute(Cronet_ExecutorPtr self,
                                Cronet_RunnablePtr command) {
  Cronet_ExecutorContext context = Cronet_Executor_GetContext(self);
  SequencedExecutor* executor = static_cast<SequencedExecutor*>(context);
  CHECK(self);
  DLOG(INFO) << "Post Task";
  executor->task_runner_->PostTask(FROM_HERE,
                                   base::BindOnce(
                                       [](Cronet_RunnablePtr command) {
                                         DLOG(INFO) << "Running posted task";
                                         Cronet_Runnable_Run(command);
                                         DLOG(INFO) << "Destroying command";
                                         Cronet_Runnable_Destroy(command);
                                       },
                                       command));
}

Cronet_ExecutorPtr SequencedExecutor_Create() {
  return SequencedExecutor::Create();
}

}  // namespace

class ExecutorsTest : public ::testing::Test {
 protected:
  void SetUp() override {}

  void TearDown() override {}

  ExecutorsTest() {}
  ~ExecutorsTest() override {}

 public:
  bool Runnable_called_ = false;

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
  test->Runnable_called_ = true;
  DLOG(INFO) << "Hello from TestRunnable_Run";
}

// Test that DirectExecutor runs the runnable.
TEST_F(ExecutorsTest, TestDirect) {
  CHECK(!Runnable_called_);
  Cronet_RunnablePtr runnable = Cronet_Runnable_CreateStub(TestRunnable_Run);
  Cronet_Runnable_SetContext(runnable, this);
  Cronet_ExecutorPtr executor = DirectExecutor_Create();
  Cronet_Executor_Execute(executor, runnable);
  Cronet_Executor_Destroy(executor);
  CHECK(Runnable_called_);
}

// Test that PostTaskExecutor runs the runnable.
TEST_F(ExecutorsTest, TestPostTaskExecutor) {
  CHECK(!Runnable_called_);
  Cronet_RunnablePtr runnable = Cronet_Runnable_CreateStub(TestRunnable_Run);
  Cronet_Runnable_SetContext(runnable, this);
  Cronet_ExecutorPtr executor = PostTaskExecutor_Create();
  Cronet_Executor_Execute(executor, runnable);
  Cronet_Executor_Destroy(executor);
  scoped_task_environment_.RunUntilIdle();
  CHECK(Runnable_called_);
}

// Test that SequencedExecutor runs the runnable.
TEST_F(ExecutorsTest, TestSequencedExecutor) {
  CHECK(!Runnable_called_);
  Cronet_RunnablePtr runnable = Cronet_Runnable_CreateStub(TestRunnable_Run);
  Cronet_Runnable_SetContext(runnable, this);
  Cronet_ExecutorPtr executor = SequencedExecutor_Create();
  Cronet_Executor_Execute(executor, runnable);
  Cronet_Executor_Destroy(executor);
  scoped_task_environment_.RunUntilIdle();
  CHECK(Runnable_called_);
}
