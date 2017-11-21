// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_RENDERER_SERVICE_WORKER_SERVICE_WORKER_EVENT_TIMER_H_
#define CONTENT_RENDERER_SERVICE_WORKER_SERVICE_WORKER_EVENT_TIMER_H_

#include <map>

#include "base/callback.h"
#include "base/containers/queue.h"
#include "base/time/time.h"
#include "base/timer/timer.h"
#include "content/common/content_export.h"

namespace base {

class TickClock;

}  // namespace base

namespace content {

// Manages idle timeout of events.
// TODO(crbug.com/774374) Implement long running timer too.
class CONTENT_EXPORT ServiceWorkerEventTimer {
 public:
  // |idle_callback| will be called when a certain period has passed since the
  // last event ended.
  explicit ServiceWorkerEventTimer(base::RepeatingClosure idle_callback);
  // For testing.
  ServiceWorkerEventTimer(base::RepeatingClosure idle_callback,
                          std::unique_ptr<base::TickClock> tick_clock);
  ~ServiceWorkerEventTimer();

  int StartEvent(base::OnceCallback<void(int /* event_id */)> abort_callback);
  void EndEvent(int event_id);

  // Idle timeout duration since the last event has finished.
  static constexpr base::TimeDelta kIdleDelay =
      base::TimeDelta::FromSeconds(30);
  static constexpr base::TimeDelta kEventTimeout =
      base::TimeDelta::FromMinutes(5);
  static constexpr base::TimeDelta kUpdateInterval =
      base::TimeDelta::FromSeconds(30);

 private:
  void UpdateStatus();

  // For long standing event timeouts.
  std::map<int /* event_id */, base::OnceClosure> abort_callbacks_;
  base::queue<std::pair<base::TimeTicks, int /* event_id */>>
      event_timeout_times_;

  // For idle timeouts.
  // |idle_time_| will be null if there are inflight events.
  base::TimeTicks idle_time_;
  base::RepeatingClosure idle_callback_;

  // |timer_| invokes UpdateEventStatus() periodically.
  base::RepeatingTimer timer_;

  std::unique_ptr<base::TickClock> tick_clock_;
};

}  // namespace content

#endif  // CONTENT_RENDERER_SERVICE_WORKER_SERVICE_WORKER_EVENT_TIMER_H_
