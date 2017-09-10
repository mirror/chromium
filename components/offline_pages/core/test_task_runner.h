// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_OFFLINE_PAGES_CORE_TEST_TASK_RUNNER_H_
#define COMPONENTS_OFFLINE_PAGES_CORE_TEST_TASK_RUNNER_H_

#include <memory>

#include "base/macros.h"
#include "base/test/test_simple_task_runner.h"

namespace offline_pages {
class Task;

// Tool for running tasks in test.
class TestTaskRunner {
 public:
  explicit TestTaskRunner(
      scoped_refptr<base::TestSimpleTaskRunner> task_runner);
  ~TestTaskRunner();

  // Runs task with expectation that it correctly completes.
  // Task is also cleaned up after completing.
  void RunTask(std::unique_ptr<Task> task);

 private:
  scoped_refptr<base::TestSimpleTaskRunner> task_runner_;

  DISALLOW_COPY_AND_ASSIGN(TestTaskRunner);
};

}  // namespace offline_pages

#endif  // COMPONENTS_OFFLINE_PAGES_CORE_TEST_TASK_RUNNER_H_
