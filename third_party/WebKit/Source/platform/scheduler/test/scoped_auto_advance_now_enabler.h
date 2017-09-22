// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_WEBKIT_SOURCE_PLATFORM_SCHEDULER_TEST_SCOPED_AUTO_ADVANCE_NOW_ENABLER_H_
#define THIRD_PARTY_WEBKIT_SOURCE_PLATFORM_SCHEDULER_TEST_SCOPED_AUTO_ADVANCE_NOW_ENABLER_H_

#include "base/macros.h"
#include "base/memory/ref_counted.h"

namespace cc {
class OrderedSimpleTaskRunner;
}  // namespace cc

namespace blink {
namespace scheduler {

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

#endif  // THIRD_PARTY_WEBKIT_SOURCE_PLATFORM_SCHEDULER_TEST_SCOPED_AUTO_ADVANCE_NOW_ENABLER_H_
