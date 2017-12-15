// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/arc/timer/arc_timer.h"

#include <stdlib.h>
#include <unistd.h>

#include <utility>
#include <vector>

#include "base/bind.h"
#include "base/logging.h"
#include "base/posix/unix_domain_socket.h"
#include "base/task_scheduler/post_task.h"
#include "base/threading/thread_restrictions.h"

namespace arc {

ArcTimer::ArcTimer(base::ScopedFD expiration_fd,
                   mojo::InterfaceRequest<mojom::Timer> request)
    : binding_(this, std::move(request)),
      expiration_fd_(std::move(expiration_fd)),
      weak_factory_(this) {
  binding_.set_connection_error_handler(base::BindOnce(
      &ArcTimer::HandleConnectionError, weak_factory_.GetWeakPtr()));
}

ArcTimer::~ArcTimer() = default;

void ArcTimer::Start(base::TimeTicks absolute_expiration_time,
                     StartCallback callback) {
  // Start the timer to expire at |absolute_expiration_time|. This call
  // automatically overrides the previous timer set.
  //
  // If the firing time has expired then set the timer to expire immediately.
  base::TimeTicks current_time = base::TimeTicks::Now();
  base::TimeDelta delay;
  if (absolute_expiration_time > current_time) {
    delay = base::TimeDelta(absolute_expiration_time - current_time);
  }
  DVLOG(1) << "CurrentTime: " << base::Time::Now()
           << " NextAlarmAt: " << base::Time::Now() + delay;
  timer_.Start(FROM_HERE, delay,
               base::Bind(&ArcTimer::OnExpiration, weak_factory_.GetWeakPtr()));
  std::move(callback).Run(arc::mojom::ArcTimerResult::SUCCESS);
}

void ArcTimer::HandleConnectionError() {
  // Stop any pending timers when connection with the instance is dropped.
  timer_.Stop();
}

void ArcTimer::OnExpiration() {
  DVLOG(1) << "Expiration callback";
  base::AssertBlockingAllowed();
  // The instance expects 8 bytes on the read end similar to what happens on a
  // timerfd expiration. This value should represent the number of expirations.
  const uint64_t num_expirations = 1;
  if (!base::UnixDomainSocket::SendMsg(expiration_fd_.get(), &num_expirations,
                                       sizeof(num_expirations),
                                       std::vector<int>())) {
    LOG(ERROR) << "Failed to indicate timer expiry to the instance";
  }
}

}  // namespace arc
