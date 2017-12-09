// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <mojo/edk/embedder/embedder.h>
#include <mojo/edk/embedder/scoped_platform_handle.h>
#include <sys/socket.h>
#include <sys/types.h>

#include <memory>
#include <utility>

#include "base/bind.h"
#include "base/files/file_descriptor_watcher_posix.h"
#include "base/files/scoped_file.h"
#include "base/posix/unix_domain_socket.h"
#include "base/run_loop.h"
#include "base/time/time.h"
#include "components/arc/test/fake_timer_instance.h"
#include "components/arc/timer/arc_timer_request.h"
#include "components/arc/timer/arc_timer_traits.h"

namespace arc {

struct FakeTimerInstance::ArcTimerInfo {
  ArcTimerInfo() : timer(nullptr) {}
  ArcTimerInfo(ArcTimerInfo&&) = default;
  ~ArcTimerInfo() = default;

  // The mojom::Timer associated with an arc timer.
  mojom::TimerPtr timer;

  // The fd that will be signalled by the host when the timer expires.
  base::ScopedFD read_fd;
};

FakeTimerInstance::FakeTimerInstance() : weak_ptr_factory_(this) {}

FakeTimerInstance::~FakeTimerInstance() = default;

void FakeTimerInstance::Init(mojom::TimerHostPtr host_ptr,
                             InitCallback callback) {
  host_ptr_ = std::move(host_ptr);
  std::move(callback).Run();
}

void FakeTimerInstance::CallCreateTimers(const std::vector<clockid_t>& clocks) {
  std::vector<arc::ArcTimerRequest> arc_timer_requests;
  for (const clockid_t& clock : clocks) {
    arc::ArcTimerRequest request;
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
          [](FakeTimerInstance* instance, base::OnceClosure quit_callback,
             base::Optional<std::vector<mojom::ArcTimerResponsePtr>> result) {
            instance->CreateTimersCallback(std::move(result));
            std::move(quit_callback).Run();
          },
          base::Unretained(this), loop.QuitClosure()));
  loop.Run();
}

bool FakeTimerInstance::CallStartTimer(clockid_t clock_id,
                                       int64_t nanoseconds_from_now) {
  if (!IsTimerPresent(clock_id)) {
    LOG(ERROR) << "Timer of type: " << clock_id << " not present";
    return false;
  }

  // Start timer to expire at |nanoseconds_from_now|.
  const mojom::TimerPtr& timer = arc_timers_[clock_id].timer;
  base::RunLoop loop;
  bool success = true;
  LOG(INFO) << "Start call time: " << base::Time::Now()
            << " Expected expiration time: "
            << base::Time::Now() + base::TimeDelta::FromMicroseconds(
                                       nanoseconds_from_now /
                                       base::Time::kNanosecondsPerMicrosecond);
  timer->Start(
      nanoseconds_from_now,
      base::BindOnce(
          [](base::RunLoop* loop, bool* success, mojom::ArcTimerResult result) {
            *success = (result == mojom::ArcTimerResult::SUCCESS);
            loop->Quit();
          },
          &loop, &success));
  loop.Run();
  if (!success) {
    LOG(ERROR) << "Start timer failed";
    return false;
  }
  return true;
}

bool FakeTimerInstance::WaitForExpiration(clockid_t clock_id) {
  if (!IsTimerPresent(clock_id)) {
    LOG(ERROR) << "Timer of type: " << clock_id << " not present";
    return false;
  }

  // Wait for the host to indicate expiration by watching the read end of the
  // socket pair.
  base::RunLoop loop;
  std::unique_ptr<base::FileDescriptorWatcher::Controller>
      watch_readable_controller = base::FileDescriptorWatcher::WatchReadable(
          arc_timers_[clock_id].read_fd.get(), loop.QuitClosure());
  loop.Run();

  // Once the watcher returns check if the expiration happened at
  // |nanoseconds_from_now| ns from the time the call was scheduled. The timer
  // expects 8 bytes to be written from the host upon expiration.
  uint64_t timer_data;
  std::vector<base::ScopedFD> fds;
  ssize_t bytes_read =
      base::UnixDomainSocket::RecvMsg(arc_timers_[clock_id].read_fd.get(),
                                      &timer_data, sizeof(timer_data), &fds);
  if (bytes_read < static_cast<ssize_t>(sizeof(timer_data))) {
    LOG(ERROR) << "Incorrect timer wake up bytes_read: " << bytes_read;
    return false;
  }
  LOG(INFO) << "Actual expiration time: " << base::Time::Now();
  return true;
}

bool FakeTimerInstance::IsTimerPresent(const clockid_t clock_id) {
  return arc_timers_.find(clock_id) != arc_timers_.end();
}

void FakeTimerInstance::CreateTimersCallback(
    base::Optional<std::vector<mojom::ArcTimerResponsePtr>> result) {
  if (result == base::nullopt) {
    LOG(ERROR) << "Null timer objects array returned";
    arc_timers_.clear();
    return;
  }

  const auto& responses = result.value();
  if (responses.size() != arc_timers_.size()) {
    LOG(ERROR) << "Incorrect number of timer objects returned: "
               << responses.size();
    arc_timers_.clear();
    return;
  }

  // Store the timer objects. These will be retrieved to set timers.
  for (auto& response : result.value()) {
    int32_t clock_id;
    if (!mojo::EnumTraits<arc::mojom::ClockId, int32_t>::FromMojom(
            response->clock_id, &clock_id)) {
      LOG(ERROR) << "Failed to convert mojo clock id: " << response->clock_id;
      arc_timers_.clear();
      return;
    }
    LOG(INFO) << "Mojo Id: " << (int32_t)response->clock_id
              << " Clock Id: " << clock_id;
    auto& arc_timer_info = arc_timers_[clock_id];
    arc_timer_info.timer = mojom::TimerPtr(std::move(response->timer));
  }
}

}  // namespace arc
