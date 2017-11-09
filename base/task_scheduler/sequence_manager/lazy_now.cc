// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/task_scheduler/sequence_manager/lazy_now.h"

#include "base/time/tick_clock.h"
#include "base/task_scheduler/sequence_manager/task_queue_manager.h"

namespace base {

base::TimeTicks LazyNow::Now() {
  if (now_.is_null())
    now_ = tick_clock_->NowTicks();
  return now_;
}


}  // namespace base
