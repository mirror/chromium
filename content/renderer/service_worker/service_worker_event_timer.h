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

// Manages timeout for each event and the idle timeout.
class CONTENT_EXPORT ServiceWorkerEventTimer {
 public:
  // |idle_callback| will be called when a certain period has passed since the
  // last event ended.
  explicit ServiceWorkerEventTimer(base::RepeatingClosure idle_callback);
  // For testing.
  ServiceWorkerEventTimer(base::RepeatingClosure idle_callback,
                          std::unique_ptr<base::TickClock> tick_clock);
  ~ServiceWorkerEventTimer();

  // |abort_callback| will be called when |kEventTimeout| has passed since
  // StartEvent() was called, or EndEvent() has not been called when the timer
  // is destructed.
  int StartEvent(base::OnceCallback<void(int /* event_id */)> abort_callback);
  void EndEvent(int event_id);

  // Idle timeout duration since the last event has finished.
  static constexpr base::TimeDelta kIdleDelay =
      base::TimeDelta::FromSeconds(30);
  // Duration of the long standing event timeout since StartEvent() has been
  // called.
  static constexpr base::TimeDelta kEventTimeout =
      base::TimeDelta::FromMinutes(5);
  // ServiceWorkerEventTimer periodically updates the timeout state by
  // kUpdateInterval.
  static constexpr base::TimeDelta kUpdateInterval =
      base::TimeDelta::FromSeconds(30);

 private:
  void UpdateStatus();

  // For long standing event timeouts.
  // Contains only the inflight events. The abort callback should be removed
  // when EndEvent() is called.
  std::map<int /* event_id */, base::OnceClosure> abort_callbacks_;

  // For long standing event timeouts.
  // Contains all of timeout times. The timed out event id may not exist in
  // |abort_callbacks_|. That means the event has been successfully finished.
  base::queue<std::pair<base::TimeTicks, int /* event_id */>>
      event_timeout_times_;

  // For idle timeouts.
  // |idle_time_| will be null if there are inflight events.
  base::TimeTicks idle_time_;

  // For idle timeouts.
  // Invoked when UpdateStatus() is called after |idle_time_|.
  base::RepeatingClosure idle_callback_;

  // |timer_| invokes UpdateEventStatus() periodically.
  base::RepeatingTimer timer_;

  std::unique_ptr<base::TickClock> tick_clock_;
};

}  // namespace content

#endif  // CONTENT_RENDERER_SERVICE_WORKER_SERVICE_WORKER_EVENT_TIMER_H_
