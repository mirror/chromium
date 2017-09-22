// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_WEBKIT_SOURCE_PLATFORM_SCHEDULER_TEST_TEST_UTILS_H_
#define THIRD_PARTY_WEBKIT_SOURCE_PLATFORM_SCHEDULER_TEST_TEST_UTILS_H_

#include <string>
#include <vector>
#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "base/single_thread_task_runner.h"

namespace cc {
class OrderedSimpleTaskRunner;
}  // namespace cc

namespace blink {
namespace scheduler {

void AppendToVectorTestTask(std::vector<std::string>* vector,
                            std::string value);
void AppendToVectorIdleTestTask(std::vector<std::string>* vector,
                                std::string value,
                                base::TimeTicks deadline);
void AppendToVectorReentrantTask(base::SingleThreadTaskRunner* task_runner,
                                 std::vector<int>* vector,
                                 int* reentrant_count,
                                 int max_reentrant_count);

inline bool MessageLoopTaskCounter(size_t* count) {
  *count = *count + 1;
  return true;
}

inline void NopTask() {}

// RAII helper class to enable auto advancing of time inside mock task runner.
// Automatically disables auto-advancement when destroyed.
class ScopedAutoAdvanceNowEnabler {
 public:
  ScopedAutoAdvanceNowEnabler(
      scoped_refptr<cc::OrderedSimpleTaskRunner> task_runner);
  ~ScopedAutoAdvanceNowEnabler();

 private:
  scoped_refptr<cc::OrderedSimpleTaskRunner> task_runner_;

  DISALLOW_COPY_AND_ASSIGN(ScopedAutoAdvanceNowEnabler);
};

}  // namespace scheduler
}  // namespace blink

#endif  // THIRD_PARTY_WEBKIT_SOURCE_PLATFORM_SCHEDULER_TEST_TEST_UTILS_H_
