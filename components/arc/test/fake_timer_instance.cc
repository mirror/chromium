// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <mojo/edk/embedder/embedder.h>
#include <mojo/edk/embedder/scoped_platform_handle.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <utility>

#include "base/bind.h"
#include "base/files/scoped_file.h"
#include "base/posix/unix_domain_socket.h"
#include "base/run_loop.h"
#include "base/time/time.h"
#include "components/arc/test/fake_timer_instance.h"
#include "components/arc/timer/arc_timer_request.h"

namespace arc {

void FakeTimer::Start(int64_t nanoseconds_from_now, StartCallback callback) {}

FakeTimerInstance::FakeTimerInstance() : weak_ptr_factory_(this) {}

FakeTimerInstance::~FakeTimerInstance() = default;

void FakeTimerInstance::Init(mojom::TimerHostPtr host_ptr) {
  host_ptr_ = std::move(host_ptr);
}

void FakeTimerInstance::CallCreateTimers(const std::vector<clockid_t>& clocks) {
  // Store the clocks sent by the caller. This will be used to associate timer
  // objects returned by the host to a clock id.
  std::vector<clockid_t> arc_clocks = clocks;

  std::vector<arc::ArcTimerRequest> arc_timer_requests;
  arc::ArcTimerRequest request;
  for (const clockid_t& clock : arc_clocks) {
    request.clock_id = clock;
    // Create a socket pair for each clock. One socket will be part of the mojo
    // argument and will be used by the host to indicate when the timer
    // expires. The other socket will be stored in the |fds| array, it can only
    // be used to detect the expiration of the timer by epolling and
    // reading.
    base::ScopedFD read_fd;
    base::ScopedFD write_fd;
    if (!base::CreateSocketPair(&read_fd, &write_fd)) {
      LOG(ERROR) << "Failed to create socket pair for ARC timers";
      return;
    }
    request.expiration_fd = std::move(write_fd);
    // TODO: Check for handle validity.
    // Create an entry for each clock in the |arc_timers_| map. The timer object
    // will be populated in the callback to the call to create timers.
    ArcTimerInfo arc_timer_info;
    arc_timer_info.read_fd = std::move(read_fd);
    auto emplace_result = arc_timers_.emplace(clock, std::move(arc_timer_info));
    if (!emplace_result.second) {
      LOG(ERROR) << "Failed to create timer entry";
      arc_timers_.clear();
      return;
    }
    arc_timer_requests.push_back(std::move(request));
  }

  // Create timers corresponding to |arc_timer_requests|.
  base::RunLoop loop;
  host_ptr_->CreateTimers(
      std::move(arc_timer_requests),
      base::BindOnce(
          [](FakeTimerInstance* instance, std::vector<clockid_t> arc_clocks,
             base::RunLoop* loop,
             base::Optional<std::vector<mojom::TimerPtr>> result) {
            instance->CreateTimersCallback(arc_clocks, loop, &result);
          },
          base::Unretained(this), std::move(arc_clocks), &loop));
  loop.Run();
}

bool FakeTimerInstance::CallStartTimerAndWaitForExpiration(
    clockid_t clock_id,
    int64_t nanoseconds_from_now) {
  if (!IsTimerPresent(clock_id)) {
    LOG(ERROR) << "Timer of type: " << clock_id << " not present";
    return false;
  }
  const mojom::TimerPtr& timer = arc_timers_[clock_id].timer;
  // Start timer to expire at |nanoseconds_from_now|.
  base::RunLoop loop;
  bool status = true;
  LOG(ERROR) << "Start call time: " << base::Time::Now();
  /*
  base::Time expected_expiration_time =
      base::Time::Now() +
      base::TimeDelta::FromMicroseconds(nanoseconds_from_now /
      base::Time::kNanosecondsPerMicrosecond);
  */
  DCHECK(timer);
  timer->Start(
      nanoseconds_from_now,
      base::BindOnce(
          [](base::RunLoop* loop, bool* status, mojom::ArcTimerResult result) {
            if (result == mojom::ArcTimerResult::FAILURE) {
              *status = false;
            } else {
              *status = true;
            }
            LOG(ERROR) << "Status is: " << *status;
            loop->Quit();
          },
          &loop, &status));
  loop.Run();
  if (!status) {
    return false;
  }
  // If the call succeeded then wait for the host to indicate expiration and
  // check if if happened at |nanoseconds_from_now| ns from the time the call
  // was scheduled.
  //
  // The timer expects 8 bytes to be written from the host upon expiration.
  uint64_t timer_data;
  size_t bytes_expected = sizeof(timer_data);
  ssize_t bytes_read;
  std::vector<base::ScopedFD> fds;
  if ((bytes_read = base::UnixDomainSocket::RecvMsg(
           arc_timers_[clock_id].read_fd.get(), &timer_data, bytes_expected,
           &fds)) < static_cast<ssize_t>(bytes_expected)) {
    LOG(ERROR) << "Incorrect timer wake up bytes_read: " << bytes_read;
    return false;
  }
  /*
  // Check if the wake up happened within an admissible error limit = 1ms.
  constexpr int64_t max_expiration_error_ns = 1000000;
  if (!((expected_expiration_time - base::Time::Now()) <
        base::TimeDelta::FromMicroseconds(
            max_expiration_error_ns /
            base::Time::kNanosecondsPerMicrosecond))) {
    return false;
  }
  */
  LOG(ERROR) << "Expiration time: " << base::Time::Now();
  return true;
}  // namespace arc

bool FakeTimerInstance::IsTimerPresent(const clockid_t clock_id) {
  return arc_timers_.find(clock_id) != arc_timers_.end();
}

FakeTimerInstance::ArcTimerInfo::ArcTimerInfo() : timer(nullptr), read_fd(-1) {}
FakeTimerInstance::ArcTimerInfo::ArcTimerInfo(ArcTimerInfo&&) = default;
FakeTimerInstance::ArcTimerInfo::~ArcTimerInfo() = default;

void FakeTimerInstance::CreateTimersCallback(
    const std::vector<clockid_t>& arc_clocks,
    base::RunLoop* loop,
    base::Optional<std::vector<mojom::TimerPtr>>* result) {
  if (*result == base::nullopt) {
    LOG(ERROR) << "Null timer objects array returned";
    arc_timers_.clear();
    loop->Quit();
    return;
  }

  std::vector<mojom::TimerPtr> result_timers;
  for (auto& timer : result->value()) {
    result_timers.push_back(std::move(timer));
  }

  if (result_timers.empty()) {
    LOG(ERROR) << "No timer objects returned";
    arc_timers_.clear();
    loop->Quit();
    return;
  }

  if (result_timers.size() != arc_clocks.size()) {
    LOG(ERROR) << "Incorrect number of timer objects returned";
    arc_timers_.clear();
    loop->Quit();
    return;
  }

  // Store the timer objects in their respective entries in |arc_timers_|. They
  // will be used during a |Start| call.
  for (size_t i = 0; i < arc_clocks.size(); i++) {
    auto& arc_timer_info = arc_timers_[arc_clocks[i]];
    arc_timer_info.timer = std::move(result_timers[i]);
  }
  loop->Quit();
}

}  // namespace arc

