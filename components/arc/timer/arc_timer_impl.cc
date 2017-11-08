// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/arc/timer/arc_timer_impl.h"

#include <fcntl.h>
#include <signal.h>
#include <stdlib.h>
#include <unistd.h>

#include <memory>
#include <utility>

#include <base/memory/ptr_util.h>

namespace arc {

void ArcTimer::ExpirationCallback() {
  // The instance expects 8 bytes on the read end, similar to what happens on a
  // timerfd expiration.
  uint64_t dummy_data = 0xAB;
  ssize_t bytes_to_write = sizeof(dummy_data);
  if (write(expiration_fd_.get(), &dummy_data, bytes_to_write) <
      bytes_to_write) {
    LOG(ERROR) << "Failed to indicate timer expiry to Host";
  }
}

void ArcTimer::SetBinding(
    mojo::InterfaceRequest<mojom::Timer> interface_request) {
  binding_.Bind(std::move(interface_request));
}

void ArcTimer::Start(int64_t seconds_from_now,
                     int64_t nanoseconds_from_now,
                     StartCallback callback) {
  // Start the timer with the new time period given. This call automatically
  // overrides the previous timer set.
  base::TimeDelta delay =
      base::TimeDelta::FromSeconds(seconds_from_now) +
      base::TimeDelta::FromMicroseconds(nanoseconds_from_now /
                                        base::Time::kNanosecondsPerMicrosecond);
  DVLOG(1) << "CurrentTime: " << base::Time::Now()
           << " NextAlarmAt: " << base::Time::Now() + delay;
  DCHECK(timer_);
  timer_->Start(
      FROM_HERE, delay,
      base::Bind(&ArcTimer::ExpirationCallback, base::Unretained(this)));
  std::move(callback).Run(arc::mojom::ArcTimerResult::SUCCESS);
}

ArcTimer::~ArcTimer() {
  if (timer_) {
    timer_->Stop();
  }
}

ArcTimerImpl::ArcTimerImpl()
    : arc_supported_timer_clocks_(
          {CLOCK_REALTIME_ALARM, CLOCK_BOOTTIME_ALARM}) {}

// Reads the clock types sent by the instance and creates an alarm corresponding
// to each clock. Each clock has only one timer associated with it.
std::vector<mojom::TimerPtr> ArcTimerImpl::CreateArcTimers(
    std::vector<arc::ArcTimerRequest> arc_timer_requests) {
  DVLOG(1) << "Received request to create ARC timers";

  std::vector<mojom::TimerPtr> result;
  // TODO: Handle double calls.

  // Iterate over the list of {clock_id, expiration_fd} and create an |ArcTimer|
  // entry for each clock.
  for (auto& request : arc_timer_requests) {
    // Read each entry one by one. Each entry will have an |ArcTimer| entry
    // associated with it.
    int32_t clock_id = request.clock_id;

    // If any clock is not supported then return an error.
    if (arc_supported_timer_clocks_.find(clock_id) ==
        arc_supported_timer_clocks_.end()) {
      LOG(ERROR) << "Unsupported clock=" << clock_id;
      result.clear();
      return result;
    }

    // TODO: Duplicate clock check.

    base::ScopedFD expiration_fd = std::move(request.expiration_fd);
    if (!expiration_fd.is_valid()) {
      LOG(ERROR) << "Bad file descriptor for clock=" << clock_id;
      result.clear();
      return result;
    }

    // TODO(abhishekbh): Modify SimpleAlarmTimer to take |clock_id|.
    ArcTimer* arc_timer = new ArcTimer(
        std::move(expiration_fd), std::make_unique<timers::SimpleAlarmTimer>());
    mojom::TimerPtr timer_proxy;
    arc_timer->SetBinding(mojo::MakeRequest(&timer_proxy));
    result.push_back(std::move(timer_proxy));
  }

  // The |expiration_fd| is the write end of a pipe. If the read end in
  // the instance is closed then writing to that fd will cause a SIGPIPE to be
  // delivered that will crash this daemon. Ignore it to prevent that.
  signal(SIGPIPE, SIG_IGN);
  return result;
}

}  // namespace arc
