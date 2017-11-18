// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/arc/timer/arc_timer.h"

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
  base::PostTaskWithTraits(
      FROM_HERE, {base::MayBlock(), base::TaskPriority::USER_BLOCKING},
      base::BindOnce(&ArcTimer::WriteExpiration, weak_factory_.GetWeakPtr()));
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

void ArcTimer::Start(int64_t nanoseconds_from_now, StartCallback callback) {
  // Start the timer with the new time period given. This call automatically
  // overrides the previous timer set.
  base::TimeDelta delay = base::TimeDelta::FromMicroseconds(
      nanoseconds_from_now / base::Time::kNanosecondsPerMicrosecond);
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

}  // namespace arc
