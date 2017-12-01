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
#include "base/run_loop.h"
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
    LOG(ERROR) << "Clock Id: " << clock;
    request.clock_id = clock;
    // Create a socket pair for each clock. One socket will be part of the mojo
    // argument and will be used by the host to indicate when the timer
    // expires. The other socket will be stored in the |fds| array, it can only
    // be used to detect the expiration of the timer by epolling and
    // reading.
    int socket_pair[2];
    if (socketpair(AF_UNIX, SOCK_SEQPACKET, 0, socket_pair) < 0) {
      LOG(ERROR) << "Failed to create socket pair for ARC timers";
      return;
    }
    request.expiration_fd = base::ScopedFD(socket_pair[1]);
    // TODO: Check for handle validity.
    // TODO: Store socket_pair[0] in epoll set.
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

bool FakeTimerInstance::IsTimerPresent(const clockid_t clock) {
  return arc_timers_.find(clock) != arc_timers_.end();
}

void FakeTimerInstance::CreateTimersCallback(
    const std::vector<clockid_t>& arc_clocks,
    base::RunLoop* loop,
    base::Optional<std::vector<mojom::TimerPtr>>* result) {
  if (*result == base::nullopt) {
    LOG(ERROR) << "Null timer objects array returned";
    loop->Quit();
    return;
  }

  std::vector<mojom::TimerPtr> result_timers;
  for (auto& timer : result->value()) {
    result_timers.push_back(std::move(timer));
  }

  if (result_timers.empty()) {
    LOG(ERROR) << "No timer objects returned";
    loop->Quit();
    return;
  }

  if (result_timers.size() != arc_clocks.size()) {
    LOG(ERROR) << "Incorrect number of timer objects returned";
    loop->Quit();
    return;
  }

  // Store the timer objects in a map to be used during a |Start|
  // call.
  for (size_t i = 0; i < arc_clocks.size(); i++) {
    auto emplace_result =
        arc_timers_.emplace(arc_clocks[i], std::move(result_timers[i]));
    if (!emplace_result.second) {
      LOG(ERROR) << "Failed to store timer";
      arc_timers_.clear();
      loop->Quit();
      return;
    }
  }
  loop->Quit();
}

}  // namespace arc

