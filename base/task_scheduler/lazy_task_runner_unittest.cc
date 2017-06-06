// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "testing/gtest/include/gtest/gtest.h"

#include "base/task_scheduler/lazy_task_runner.h"
#include "base/test/scoped_task_environment.h"

namespace base {

namespace {

LazySequencedTaskRunner g_sequenced_task_runner = {{MayBlock()}, 0};

class LazyTaskRunnerTest : public testing::Test {
 protected:
  test::ScopedTaskEnvironment scoped_task_environment;

 private:
  DISALLOW_COPY_AND_ASSIGN(LazyTaskRunnerTest);
};

}  // namespace

TEST(LazyTaskRunner, LazySequencedTaskRunner) {
  auto v = g_task_runner.Get();
  LOG(ERROR) << v.get();
}

}  // namespace base
