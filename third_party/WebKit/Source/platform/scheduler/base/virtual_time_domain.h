// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_WEBKIT_SOURCE_PLATFORM_SCHEDULER_BASE_VIRTUAL_TIME_DOMAIN_H_
#define THIRD_PARTY_WEBKIT_SOURCE_PLATFORM_SCHEDULER_BASE_VIRTUAL_TIME_DOMAIN_H_

#include "base/callback.h"
#include "base/macros.h"
#include "base/threading/thread_checker.h"
#include "platform/scheduler/base/time_domain.h"

namespace blink {
namespace scheduler {

class PLATFORM_EXPORT VirtualTimeDomain : public TimeDomain {
 public:
  VirtualTimeDomain(base::Time initial_time,
                    base::TimeTicks initial_time_ticks);
  ~VirtualTimeDomain() override;

  // TimeDomain implementation:
  LazyNow CreateLazyNow() const override;
  base::TimeTicks Now() const override;
  base::Optional<base::TimeDelta> DelayTillNextTask(LazyNow* lazy_now) override;
  const char* GetName() const override;

  base::Time Date() const;

  // Advances this time domain to |now|. NOTE |now| is supposed to be
  // monotonically increasing.  NOTE it's the responsibility of the caller to
  // call TaskQueueManager::MaybeScheduleImmediateWork if needed.
  void AdvanceNowTo(base::TimeTicks now);

  void V8Initalized();

 protected:
  void OnRegisterWithTaskQueueManager(
      TaskQueueManager* task_queue_manager) override;
  void RequestWakeUpAt(base::TimeTicks now, base::TimeTicks run_time) override;
  void CancelWakeUpAt(base::TimeTicks run_time) override;
  void AsValueIntoInternal(
      base::trace_event::TracedValue* state) const override;

  void RequestDoWork();

 private:
  const base::PlatformThreadId thread_id_;
  const base::Time initial_time_;
  const base::TimeTicks initial_time_ticks_;
  base::TimeDelta min_time_offset_;

  mutable base::Lock lock_;  // Protects  everything in this block.
  base::TimeTicks now_ticks_;
  mutable base::Time previous_time_;
  bool v8_initalized_;

  TaskQueueManager* task_queue_manager_;  // NOT OWNED
  base::Closure do_work_closure_;

  DISALLOW_COPY_AND_ASSIGN(VirtualTimeDomain);
};

}  // namespace scheduler
}  // namespace blink

#endif  // THIRD_PARTY_WEBKIT_SOURCE_PLATFORM_SCHEDULER_BASE_VIRTUAL_TIME_DOMAIN_H_
