// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "platform/scheduler/test/scoped_auto_advance_now_enabler.h"

#include "components/viz/test/ordered_simple_task_runner.h"

namespace blink {
namespace scheduler {

ScopedAutoAdvanceNowEnabler::ScopedAutoAdvanceNowEnabler(
    scoped_refptr<cc::OrderedSimpleTaskRunner> task_runner)
    : task_runner_(task_runner) {
  if (task_runner_)
    task_runner_->SetAutoAdvanceNowToPendingTasks(true);
}

ScopedAutoAdvanceNowEnabler::~ScopedAutoAdvanceNowEnabler() {
  if (task_runner_)
    task_runner_->SetAutoAdvanceNowToPendingTasks(false);
}

}  // namespace scheduler
}  // namespace blink
