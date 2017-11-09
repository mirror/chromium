// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef BASE_TASK_SCHEDULER_SEQUENCE_MANAGER_REAL_TIME_DOMAIN_H_
#define BASE_TASK_SCHEDULER_SEQUENCE_MANAGER_REAL_TIME_DOMAIN_H_

#include <set>

#include "base/macros.h"
#include "base/base_export.h"
#include "base/task_scheduler/sequence_manager/time_domain.h"

namespace base {


class BASE_EXPORT RealTimeDomain : public TimeDomain {
 public:
  RealTimeDomain();
  ~RealTimeDomain() override;

  // TimeDomain implementation:
  LazyNow CreateLazyNow() const override;
  base::TimeTicks Now() const override;
  base::Optional<base::TimeDelta> DelayTillNextTask(LazyNow* lazy_now) override;
  const char* GetName() const override;

 protected:
  void OnRegisterWithTaskQueueManager(
      TaskQueueManager* task_queue_manager) override;
  void RequestWakeUpAt(base::TimeTicks now, base::TimeTicks run_time) override;
  void CancelWakeUpAt(base::TimeTicks run_time) override;
  void AsValueIntoInternal(
      base::trace_event::TracedValue* state) const override;

 private:
  TaskQueueManager* task_queue_manager_;  // NOT OWNED

  DISALLOW_COPY_AND_ASSIGN(RealTimeDomain);
};


}  // namespace base

#endif  // BASE_TASK_SCHEDULER_SEQUENCE_MANAGER_REAL_TIME_DOMAIN_H_
