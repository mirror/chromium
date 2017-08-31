// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/task_runner_util.h"

#include <utility>

#include "base/bind.h"
#include "base/location.h"
#include "base/message_loop/message_loop.h"
#include "base/run_loop.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace base {

namespace {

int ReturnFourtyTwo() {
  return 42;
}

void StoreValue(int* destination, int value) {
  *destination = value;
}

void StoreDoubleValue(double* destination, double value) {
  *destination = value;
}

int g_foo_destruct_count = 0;
int g_foo_free_count = 0;

struct Foo {
  ~Foo() {
    ++g_foo_destruct_count;
  }
};

std::unique_ptr<Foo> CreateFoo() {
  return std::unique_ptr<Foo>(new Foo);
}

void ExpectFoo(std::unique_ptr<Foo> foo) {
  EXPECT_TRUE(foo.get());
  std::unique_ptr<Foo> local_foo(std::move(foo));
  EXPECT_TRUE(local_foo.get());
  EXPECT_FALSE(foo.get());
}

struct FooDeleter {
  void operator()(Foo* foo) const {
    ++g_foo_free_count;
    delete foo;
  };
};

std::unique_ptr<Foo, FooDeleter> CreateScopedFoo() {
  return std::unique_ptr<Foo, FooDeleter>(new Foo);
}

void ExpectScopedFoo(std::unique_ptr<Foo, FooDeleter> foo) {
  EXPECT_TRUE(foo.get());
  std::unique_ptr<Foo, FooDeleter> local_foo(std::move(foo));
  EXPECT_TRUE(local_foo.get());
  EXPECT_FALSE(foo.get());
}

void AsyncTaskContinued(OnceCallback<void(int)> on_completed) {
  std::move(on_completed).Run(42);
}

void AsyncTask(scoped_refptr<TaskRunner> task_runner,
               OnceCallback<void(int)> on_completed) {
  task_runner->PostTask(FROM_HERE,
                        BindOnce(&AsyncTaskContinued, std::move(on_completed)));
}

}  // namespace

TEST(TaskRunnerHelpersTest, PostTaskAndReplyWithResult) {
  int result = 0;

  MessageLoop message_loop;
  PostTaskAndReplyWithResult(message_loop.task_runner().get(), FROM_HERE,
                             Bind(&ReturnFourtyTwo),
                             Bind(&StoreValue, &result));

  RunLoop().RunUntilIdle();

  EXPECT_EQ(42, result);
}

TEST(TaskRunnerHelpersTest, PostTaskAndReplyWithResultImplicitConvert) {
  double result = 0;

  MessageLoop message_loop;
  PostTaskAndReplyWithResult(message_loop.task_runner().get(), FROM_HERE,
                             Bind(&ReturnFourtyTwo),
                             Bind(&StoreDoubleValue, &result));

  RunLoop().RunUntilIdle();

  EXPECT_DOUBLE_EQ(42.0, result);
}

TEST(TaskRunnerHelpersTest, PostTaskAndReplyWithResultPassed) {
  g_foo_destruct_count = 0;
  g_foo_free_count = 0;

  MessageLoop message_loop;
  PostTaskAndReplyWithResult(message_loop.task_runner().get(), FROM_HERE,
                             Bind(&CreateFoo), Bind(&ExpectFoo));

  RunLoop().RunUntilIdle();

  EXPECT_EQ(1, g_foo_destruct_count);
  EXPECT_EQ(0, g_foo_free_count);
}

TEST(TaskRunnerHelpersTest, PostTaskAndReplyWithResultPassedFreeProc) {
  g_foo_destruct_count = 0;
  g_foo_free_count = 0;

  MessageLoop message_loop;
  PostTaskAndReplyWithResult(message_loop.task_runner().get(), FROM_HERE,
                             Bind(&CreateScopedFoo), Bind(&ExpectScopedFoo));

  RunLoop().RunUntilIdle();

  EXPECT_EQ(1, g_foo_destruct_count);
  EXPECT_EQ(1, g_foo_free_count);
}

TEST(TaskRunnerHelpersTest, PostAsyncTaskAndReply) {
  int result = 0;

  MessageLoop message_loop;
  PostAsyncTaskAndReply(
      message_loop.task_runner().get(), FROM_HERE,
      BindOnce(&AsyncTask, message_loop.task_runner()),
      BindOnce([](int* result, int value) { *result = value; },
               Unretained(&result)));

  RunLoop().RunUntilIdle();
  EXPECT_EQ(42, result);
}

TEST(TaskRunnerHelpersTest, PostAsyncTaskAndReply_ImplicitConvert) {
  double result = 0;

  MessageLoop message_loop;
  PostAsyncTaskAndReply(
      message_loop.task_runner().get(), FROM_HERE,
      BindOnce(&AsyncTask, message_loop.task_runner()),
      BindOnce([](double* result, double value) { *result = value; },
               Unretained(&result)));

  RunLoop().RunUntilIdle();
  EXPECT_EQ(42., result);
}

TEST(TaskRunnerHelpersTest, PostAsyncTaskAndReply_MultipleReturnValue) {
  int result1 = 0;
  double result2 = 0;

  MessageLoop message_loop;
  PostAsyncTaskAndReply(
      message_loop.task_runner().get(), FROM_HERE,
      BindOnce([](OnceCallback<void(int, double)> on_completed) {
        std::move(on_completed).Run(1, 2.5);
      }),
      BindOnce(
          [](int* result1, double* result2, int value1, double value2) {
            *result1 = value1;
            *result2 = value2;
          },
          Unretained(&result1), Unretained(&result2)));

  RunLoop().RunUntilIdle();
  EXPECT_EQ(1, result1);
  EXPECT_EQ(2.5, result2);
}

}  // namespace base
