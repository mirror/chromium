// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/renderer/service_worker/service_worker_event_timer.h"

#include "base/stl_util.h"
#include "base/time/time.h"

namespace content {

namespace {

int NextEventId() {
  static int s_next_event_id = 0;
  DCHECK_LT(s_next_event_id, std::numeric_limits<int>::max());
  return s_next_event_id++;
}

}  // namespace

// static
constexpr base::TimeDelta ServiceWorkerEventTimer::kIdleDelay;

ServiceWorkerEventTimer::ServiceWorkerEventTimer(
    base::RepeatingClosure idle_callback)
    : idle_callback_(std::move(idle_callback)) {
  // |idle_callback_| will be invoked if no event happens in |kIdleDelay|.
  idle_timer_.Start(FROM_HERE, kIdleDelay, idle_callback_);
}

ServiceWorkerEventTimer::~ServiceWorkerEventTimer() {
  // Abort all callbacks.
  for (auto& it : abort_callbacks_)
    std::move(it.second).Run();
};

int ServiceWorkerEventTimer::StartEvent(
    base::OnceCallback<void(int /* event_id */)> abort_callback) {
  idle_timer_.Stop();
  const int next_event_id = NextEventId();

  DCHECK(!base::ContainsKey(abort_callbacks_, next_event_id));
  abort_callbacks_[next_event_id] =
      base::BindOnce(std::move(abort_callback), next_event_id);
  return next_event_id;
}

void ServiceWorkerEventTimer::FinishEvent(int event_id) {
  DCHECK(base::ContainsKey(abort_callbacks_, event_id));
  abort_callbacks_.erase(event_id);
  if (abort_callbacks_.empty()) {
    idle_timer_.Start(FROM_HERE, kIdleDelay, idle_callback_);
  }
}

}  // namespace content
