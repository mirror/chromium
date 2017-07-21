// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "cc/test/test_delay_based_time_source.h"

#include "cc/test/ordered_simple_task_runner.h"

namespace cc {

TestDelayBasedTimeSource::TestDelayBasedTimeSource(
    base::SimpleTestTickClock* now_src,
    OrderedSimpleTaskRunner* task_runner)
    : DelayBasedTimeSource(task_runner), now_src_(now_src) {}

base::TimeTicks TestDelayBasedTimeSource::Now() const {
  return now_src_->NowTicks();
}

std::string TestDelayBasedTimeSource::TypeString() const {
  return "TestDelayBasedTimeSource";
}

TestDelayBasedTimeSource::~TestDelayBasedTimeSource() = default;

}  // namespace cc
