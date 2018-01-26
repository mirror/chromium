// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "net/base/test_completion_once_callback.h"

#include "base/bind.h"
#include "base/location.h"
#include "base/run_loop.h"
#include "base/single_thread_task_runner.h"
#include "base/threading/sequenced_task_runner_handle.h"
#include "net/base/completion_once_callback.h"
#include "net/base/net_errors.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace net {
namespace {

TEST(TestCompletionOnceCallbackTest, SyncCall) {
  const int kResult = 42;
  TestCompletionOnceCallback callback;
  callback.CreateCallback().Run(kResult);
  EXPECT_EQ(kResult, callback.WaitForResult());
}

TEST(TestCompletionOnceCallbackTest, AsyncCall) {
  const int kResult = 42;
  TestCompletionOnceCallback callback;
  base::SequencedTaskRunnerHandle::Get()->PostTask(
      FROM_HERE, base::BindOnce(callback.CreateCallback(), kResult));
  EXPECT_EQ(kResult, callback.WaitForResult());
}

TEST(TestCompletionOnceCallbackTest, AsyncCallAlreadyHasResult) {
  const int kResult = 42;
  TestCompletionOnceCallback callback;
  base::SequencedTaskRunnerHandle::Get()->PostTask(
      FROM_HERE, base::BindOnce(callback.CreateCallback(), kResult));
  base::RunLoop().RunUntilIdle();
  EXPECT_EQ(kResult, callback.WaitForResult());
}

TEST(TestCompletionOnceCallbackTest, GetResultWithResult) {
  const int kResult = 42;
  TestCompletionOnceCallback callback;
  EXPECT_EQ(kResult, callback.GetResult(kResult));
}

TEST(TestCompletionOnceCallbackTest, GetResultWithSyncCall) {
  const int kResult = 42;
  TestCompletionOnceCallback callback;
  callback.CreateCallback().Run(kResult);
  EXPECT_EQ(kResult, callback.GetResult(ERR_IO_PENDING));
}

TEST(TestCompletionOnceCallbackTest, GetResultWithAsyncCall) {
  const int kResult = 42;
  TestCompletionOnceCallback callback;
  base::SequencedTaskRunnerHandle::Get()->PostTask(
      FROM_HERE, base::BindOnce(callback.CreateCallback(), kResult));
  EXPECT_EQ(kResult, callback.GetResult(ERR_IO_PENDING));
}

// Check the case where a TestCompletionOnceCallback is used multiple times.
// A bit ugly, but a lot of tests do this.
TEST(TestCompletionOnceCallbackTest, Reused) {
  const int kResult1 = 1;
  TestCompletionOnceCallback callback;
  callback.CreateCallback().Run(kResult1);
  EXPECT_EQ(kResult1, callback.WaitForResult());

  const int kResult2 = 2;
  base::SequencedTaskRunnerHandle::Get()->PostTask(
      FROM_HERE, base::BindOnce(callback.CreateCallback(), kResult2));
  EXPECT_EQ(kResult2, callback.WaitForResult());

  const int kResult3 = 3;
  TestCompletionOnceCallback callback;
  base::SequencedTaskRunnerHandle::Get()->PostTask(
      FROM_HERE, base::BindOnce(callback.CreateCallback(), kResult3));
  base::RunLoop().RunUntilIdle();
  EXPECT_EQ(kResult3, callback.WaitForResult());

  // Uninvoked callbacks are allowed.
  CompletionOnceCallback unused_callback = callback.CreateCallback();
  unused_callback = callback.CreateCallback();

  const int kResult4 = 4;
  callback.CreateCallback().Run(kResult4);
  EXPECT_EQ(kResult4, callback.WaitForResult());

  const int kResult5 = 5;
  base::SequencedTaskRunnerHandle::Get()->PostTask(
      FROM_HERE, base::BindOnce(callback.CreateCallback(), kResult5));
  EXPECT_EQ(kResult5, callback.WaitForResult());
}

}  // namespace
}  // namespace net
