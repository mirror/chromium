// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CC_TEST_TEST_DELAY_BASED_TIME_SOURCE_H_
#define CC_TEST_TEST_DELAY_BASED_TIME_SOURCE_H_

#include "base/test/simple_test_tick_clock.h"
#include "cc/scheduler/delay_based_time_source.h"

namespace cc {

class OrderedSimpleTaskRunner;

class TestDelayBasedTimeSource : public DelayBasedTimeSource {
 public:
  TestDelayBasedTimeSource(base::SimpleTestTickClock* now_src,
                           OrderedSimpleTaskRunner* task_runner);
  ~TestDelayBasedTimeSource() override;

 protected:
  // Overridden from DelayBasedTimeSource
  base::TimeTicks Now() const override;
  std::string TypeString() const override;

  // Not owned.
  base::SimpleTestTickClock* now_src_;

 private:
  DISALLOW_COPY_AND_ASSIGN(TestDelayBasedTimeSource);
};

}  // namespace cc

#endif  // CC_TEST_TEST_DELAY_BASED_TEST_SOURCE_H_
