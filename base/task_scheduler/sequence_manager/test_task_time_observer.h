// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef BASE_TASK_SCHEDULER_SEQUENCE_MANAGER_TEST_TASK_TIME_OBSERVER_H_
#define BASE_TASK_SCHEDULER_SEQUENCE_MANAGER_TEST_TASK_TIME_OBSERVER_H_

#include "base/time/time.h"
#include "base/task_scheduler/sequence_manager/task_time_observer.h"

namespace base {


class TestTaskTimeObserver : public TaskTimeObserver {
 public:
  void WillProcessTask(double start_time) override {}
  void DidProcessTask(double start_time, double end_time) override {}
};


}  // namespace base

#endif  // BASE_TASK_SCHEDULER_SEQUENCE_MANAGER_TEST_TASK_TIME_OBSERVER_H_
