// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/arc/timer/arc_timer.h"

#include <stdlib.h>
#include <unistd.h>

#include <vector>

#include "base/bind.h"
#include "base/logging.h"
#include "base/posix/unix_domain_socket.h"
#include "base/task_scheduler/post_task.h"
#include "base/threading/thread_restrictions.h"
#include "base/time/time.h"

namespace {

// Callback fired when the timer in |ArcTimer| expires.
void OnExpiration(
    scoped_refptr<arc::ArcTimer::RefCountedScopedFd> expiration_fd) {
  DVLOG(1) << "Expiration callback";
  base::AssertBlockingAllowed();
  // The instance expects 8 bytes on the read end similar to what happens on a
  // timerfd expiration. This value should represent the number of expirations.
  const uint64_t num_expirations = 1;
  if (!base::UnixDomainSocket::SendMsg(expiration_fd->get(), &num_expirations,
                                       sizeof(num_expirations),
                                       std::vector<int>())) {
    LOG(ERROR) << "Failed to indicate timer expiry to the instance";
  }
}

}  // namespace

namespace arc {

void ArcTimer::Start(int64_t nanoseconds_from_now, StartCallback callback) {
  // Start the timer with the new time period given. This call automatically
  // overrides the previous timer set.
  base::TimeDelta delay = base::TimeDelta::FromMicroseconds(
      nanoseconds_from_now / base::Time::kNanosecondsPerMicrosecond);
  DVLOG(1) << "CurrentTime: " << base::Time::Now()
           << " NextAlarmAt: " << base::Time::Now() + delay;
  timer_.Start(FROM_HERE, delay, base::Bind(&OnExpiration, expiration_fd_));
  std::move(callback).Run(arc::mojom::ArcTimerResult::SUCCESS);
}

ArcTimer::RefCountedScopedFd::RefCountedScopedFd(base::ScopedFD fd)
    : fd_(std::move(fd)) {}

ArcTimer::ArcTimer(base::ScopedFD expiration_fd,
                   mojo::InterfaceRequest<mojom::Timer> request)
    : binding_(this, std::move(request)),
      expiration_fd_(new RefCountedScopedFd(std::move(expiration_fd))),
      weak_factory_(this) {
  binding_.set_connection_error_handler(base::Bind(
      &ArcTimer::ConnectionErrorHandler, weak_factory_.GetWeakPtr()));
  // Set the timer expiration callback to run on a blocking thread since
  // it will do I/O. The fastest trait option to signal to the instance is
  // |USER_BLOCKING| trait.
  timer_.SetTaskRunner(base::CreateSequencedTaskRunnerWithTraits(
      {base::MayBlock(), base::TaskPriority::USER_BLOCKING}));
}

void ArcTimer::ConnectionErrorHandler() {
  // Stop any pending timers when connection with the instance is dropped.
  timer_.Stop();
}

}  // namespace arc
