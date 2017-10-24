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

namespace {

// Copied from libbrillo::GetTimeAsLogString.
std::string GetTimeAsLogString(const base::Time& time) {
  time_t utime = time.ToTimeT();
  struct tm tm;
  CHECK_EQ(localtime_r(&utime, &tm), &tm);
  const size_t string_size = 16;
  char str[string_size];
  CHECK_EQ(strftime(str, sizeof(str), "%Y%m%d-%H%M%S", &tm), string_size - 1);
  return std::string(str);
}

}  // namespace

namespace arc_timer {

ArcTimer::~ArcTimer() {
  timer->Stop();
}

ArcTimerImpl::ArcTimerImpl()
    : arc_supported_timer_clocks_(
          {CLOCK_REALTIME_ALARM, CLOCK_BOOTTIME_ALARM}) {}

// Reads the clock types sent by the instance and creates an alarm corresponding
// to each clock. Each clock has only one timer associated with it.
arc::mojom::ArcTimerResult ArcTimerImpl::CreateArcTimers(
    std::vector<arc::ArcTimerArgs> args) {
  DVLOG(1) << "Received request to create ARC timers";

  // If this is a second create call then the instance has gone down and come
  // back up again. Cancel all previous timers and clean up open descriptors.
  if (ArcTimersInitialized()) {
    DVLOG(1) << "Clear existing ARC timers";
    arc_timer_store_.clear();
  }

  // Iterate over the list of {int clock id, fd expiry indicator fd} and create
  // an |ArcTimer| entry for each clock and store it locally.
  for (auto& arg : args) {
    // Read each entry one by one. Each entry will have an |ArcTimer| entry
    // associated with it.
    std::unique_ptr<ArcTimer> arc_timer = std::make_unique<ArcTimer>();
    int32_t clock_id = arg.clock_id;

    // If any clock is not supported then return an error.
    if (arc_supported_timer_clocks_.find(clock_id) ==
        arc_supported_timer_clocks_.end()) {
      LOG(ERROR) << "Unsupported clock=" << clock_id;
      return arc::mojom::ArcTimerResult::FAILURE;
    }

    // One clock type can only be associated with one timer. If the same clock
    // type is present more then once in the arguments then skip over it.
    if (FindArcTimer(clock_id)) {
      continue;
    }

    arc_timer->expiration_fd = std::move(arg.expiration_fd);
    if (!arc_timer->expiration_fd.is_valid()) {
      LOG(ERROR) << "Bad file descriptor for clock=" << clock_id;
      return arc::mojom::ArcTimerResult::FAILURE;
    }

    // Each clock will have a timer associated with it.
    // TODO(abhishekbh): Modify SimpleAlarmTimer to take |clock_id|.
    arc_timer->timer = std::make_unique<timers::SimpleAlarmTimer>();

    arc_timer_store_.emplace(clock_id, std::move(arc_timer));
  }

  // The |expiration_fd| is the write end of a pipe. If the read end in
  // the instance is closed then writing to that fd will cause a SIGPIPE to be
  // delivered that will crash this daemon. Ignore it to prevent that.
  signal(SIGPIPE, SIG_IGN);
  return arc::mojom::ArcTimerResult::SUCCESS;
}

// Sets time on a timer created by the instance.
arc::mojom::ArcTimerResult ArcTimerImpl::SetArcTimer(
    int32_t clock_id,
    base::TimeDelta time_delta) {
  // If a timer for the given clock is not created prior to this call then
  // return error. Else retrieve the timer associated with it.
  ArcTimer* arc_timer = FindArcTimer(clock_id);
  if (!arc_timer) {
    LOG(ERROR) << "Clock=" << clock_id << " timer not created before";
    return arc::mojom::ArcTimerResult::FAILURE;
  }

  // Start the timer with the new time period given. This call automatically
  // overrides the previous timer set.
  DVLOG(1) << "ClockId: " << clock_id
           << " CurrentTime: " << GetTimeAsLogString(base::Time::Now())
           << " NextAlarmAt: "
           << GetTimeAsLogString(base::Time::Now() + time_delta);
  arc_timer->timer->Start(FROM_HERE, time_delta,
                          base::Bind(&ArcTimerImpl::ArcTimerCallback,
                                     base::Unretained(this), clock_id));
  return arc::mojom::ArcTimerResult::SUCCESS;
}

// Stops any existing ARC timers and deletes them i.e. |kCreateArcTimersMethod|
// needs to be called again to set ARC timers.
arc::mojom::ArcTimerResult ArcTimerImpl::DeleteArcTimers() {
  DVLOG(1) << "Received request to delete ARC timers";
  arc_timer_store_.clear();
  return arc::mojom::ArcTimerResult::SUCCESS;
}

// Callback for timer of type |clock_id| set on behalf of the instance.
void ArcTimerImpl::ArcTimerCallback(int32_t clock_id) {
  DVLOG(1) << "Callback for clock=" << clock_id << " timer";
  // Write to the pipe associated with this timer. This will signal the instance
  // that a timer has fired.
  ArcTimer* arc_timer = FindArcTimer(clock_id);
  DCHECK(arc_timer);
  // The instance expects 8 bytes on the read end, similar to what happens on a
  // timerfd expiration.
  uint64_t dummy_data = 0xAB;
  ssize_t bytes_to_write = sizeof(dummy_data);
  if (write(arc_timer->expiration_fd.get(), &dummy_data, bytes_to_write) <
      bytes_to_write) {
    LOG(ERROR) << "Failed to indicate timer expiry to Host";
  }
}

// Finds the |ArcTimer| entry corresponding to |clock_id|.
ArcTimer* ArcTimerImpl::FindArcTimer(int32_t clock_id) {
  auto it = arc_timer_store_.find(clock_id);
  if (it == arc_timer_store_.end()) {
    return nullptr;
  }
  return it->second.get();
}

// Returns true if the instance has set up timers; false otherwise.
bool ArcTimerImpl::ArcTimersInitialized() const {
  return !arc_timer_store_.empty();
}

}  // namespace arc_timer
