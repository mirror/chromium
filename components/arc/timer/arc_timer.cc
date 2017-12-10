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
#include "base/time/time.h"

namespace arc {

class ArcTimer::RefCountedFd : public base::RefCountedThreadSafe<RefCountedFd> {
 public:
  static scoped_refptr<RefCountedFd> Create(base::ScopedFD fd);

  // Returns the raw file descriptor being encapsulated.
  int get() const { return fd_.get(); }

 private:
  friend class base::RefCountedThreadSafe<RefCountedFd>;
  explicit RefCountedFd(base::ScopedFD fd);
  ~RefCountedFd() = default;

  // The file descriptor being encapsulated.
  base::ScopedFD fd_;

  DISALLOW_COPY_AND_ASSIGN(RefCountedFd);
};

ArcTimer::ArcTimer(base::ScopedFD expiration_fd,
                   mojo::InterfaceRequest<mojom::Timer> request)
    : binding_(this, std::move(request)),
      expiration_fd_(RefCountedFd::Create(std::move(expiration_fd))),
      expiration_io_task_runner_(base::CreateSequencedTaskRunnerWithTraits(
          {base::MayBlock(), base::TaskPriority::USER_BLOCKING})),
      weak_factory_(this) {
  binding_.set_connection_error_handler(base::BindOnce(
      &ArcTimer::HandleConnectionError, weak_factory_.GetWeakPtr()));
}

ArcTimer::~ArcTimer() = default;

void ArcTimer::Start(int64_t nanoseconds_from_now, StartCallback callback) {
  // Start the timer with the new time period given. This call automatically
  // overrides the previous timer set.
  base::TimeDelta delay = base::TimeDelta::FromMicroseconds(
      nanoseconds_from_now / base::Time::kNanosecondsPerMicrosecond);
  DVLOG(1) << "CurrentTime: " << base::Time::Now()
           << " NextAlarmAt: " << base::Time::Now() + delay;
  timer_.Start(FROM_HERE, delay,
               base::Bind(&ArcTimer::OnExpiration, weak_factory_.GetWeakPtr(),
                          expiration_fd_));
  std::move(callback).Run(arc::mojom::ArcTimerResult::SUCCESS);
}

// static.
scoped_refptr<ArcTimer::RefCountedFd> ArcTimer::RefCountedFd::Create(
    base::ScopedFD fd) {
  return scoped_refptr<RefCountedFd>(new RefCountedFd(std::move(fd)));
}

ArcTimer::RefCountedFd::RefCountedFd(base::ScopedFD fd) : fd_(std::move(fd)) {}

void ArcTimer::HandleConnectionError() {
  // Stop any pending timers when connection with the instance is dropped.
  timer_.Stop();
}

namespace {

// Task that writes to |expiration_fd_|. Runs on |expiration_io_task_runner_|.
void WriteExpiration(scoped_refptr<ArcTimer::RefCountedFd> expiration_fd) {
  DVLOG(1) << "WriteExpiration";
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

void ArcTimer::OnExpiration(scoped_refptr<RefCountedFd> expiration_fd) {
  DVLOG(1) << "Expiration callback";
  // This callback runs on the main thread on which can't do I/O. Post a task to
  // write to |expiration_fd_|.
  expiration_io_task_runner_->PostTask(
      FROM_HERE, base::BindOnce(&WriteExpiration, expiration_fd_));
}

}  // namespace arc
