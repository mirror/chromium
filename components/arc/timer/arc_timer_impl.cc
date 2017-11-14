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

#include "base/bind.h"
#include "base/memory/ptr_util.h"
#include "base/posix/unix_domain_socket.h"
#include "base/task_scheduler/post_task.h"

namespace arc {

void ArcTimer::OnExpiration() {
  DVLOG(1) << "Expiration callback";
  // Schedule a task to indicate timer expiration to the instance. For low
  // latency signal to the instance use |USER_BLOCKING| trait.
  base::PostTaskWithTraitsAndReply(
      FROM_HERE, {base::MayBlock(), base::TaskPriority::USER_BLOCKING},
      base::Bind(&ArcTimer::WriteExpiration, weak_factory_.GetWeakPtr()),
      base::Bind(&base::DoNothing));
}

void ArcTimer::WriteExpiration() {
  DVLOG(1) << "Indicate expiration";
  // The instance expects 8 bytes on the read end similar to what happens on a
  // timerfd expiration. This value should represent the number of expirations.
  const uint64_t num_expirations = 1;
  if (!base::UnixDomainSocket::SendMsg(expiration_fd_.get(), &num_expirations,
                                       sizeof(num_expirations),
                                       std::vector<int>())) {
    LOG(ERROR) << "Failed to indicate timer expiry to the instance";
  }
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
  timer_.Start(FROM_HERE, delay,
               base::Bind(&ArcTimer::OnExpiration, base::Unretained(this)));
  std::move(callback).Run(arc::mojom::ArcTimerResult::SUCCESS);
}

ArcTimer::ArcTimer(base::ScopedFD expiration_fd,
                   mojo::InterfaceRequest<mojom::Timer> request)
    : binding_(this, std::move(request)),
      expiration_fd_(std::move(expiration_fd)),
      weak_factory_(this) {}

ArcTimer::~ArcTimer() {
  timer_.Stop();
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
  if (!arc_timers_.empty()) {
    LOG(ERROR) << "Double creation not supported";
    return result;
  }

  // Iterate over the list of {clock_id, expiration_fd} and create an |ArcTimer|
  // entry for each clock.
  std::set<int32_t> seen_clocks;
  for (auto& request : arc_timer_requests) {
    // Read each entry one by one. Each entry will have an |ArcTimer| entry
    // associated with it.
    int32_t clock_id = request.clock_id;

    // If any clock is not supported then return an error.
    if (arc_supported_timer_clocks_.find(clock_id) ==
        arc_supported_timer_clocks_.end()) {
      LOG(ERROR) << "Unsupported clock=" << clock_id;
      result.clear();
      DeleteArcTimers();
      return result;
    }

    if (seen_clocks.find(clock_id) != seen_clocks.end()) {
      LOG(ERROR) << "Duplicate clocks not supported";
      result.clear();
      DeleteArcTimers();
      return result;
    }
    seen_clocks.insert(clock_id);

    base::ScopedFD expiration_fd = std::move(request.expiration_fd);
    if (!expiration_fd.is_valid()) {
      LOG(ERROR) << "Bad file descriptor for clock=" << clock_id;
      result.clear();
      DeleteArcTimers();
      return result;
    }

    mojom::TimerPtr timer_proxy;
    mojo::InterfaceRequest<mojom::Timer> interface_request =
        mojo::MakeRequest(&timer_proxy);
    // TODO(abhishekbh): Make |ArcTimer| take |clock_id| to create timers of
    // different clock types.
    arc_timers_.insert(std::make_unique<ArcTimer>(
        std::move(expiration_fd), std::move(interface_request)));
    result.push_back(std::move(timer_proxy));
  }
  return result;
}

// Removes all timers created, implicitly stopping them.
void ArcTimerImpl::DeleteArcTimers() {
  arc_timers_.clear();
}

}  // namespace arc
