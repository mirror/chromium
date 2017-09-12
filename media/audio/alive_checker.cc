// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "media/audio/alive_checker.h"

#include <utility>

#include "base/bind.h"

namespace media {

AliveChecker::AliveChecker(
    const scoped_refptr<base::SingleThreadTaskRunner>& task_runner,
    const base::RepeatingClosure& dead_callback,
    base::TimeDelta check_interval,
    base::TimeDelta timeout,
    bool stop_at_first_alive_notification,
    bool pause_check_during_suspend)
    : check_interval_(check_interval),
      timeout_(timeout),
      task_runner_(task_runner),
      dead_callback_(std::move(dead_callback)),
      stop_at_first_alive_notification_(stop_at_first_alive_notification),
      weak_factory_(this) {
  if (pause_check_during_suspend)
    power_observer_ = std::make_unique<PowerObserverHelper>(
        task_runner_, base::Bind(&base::DoNothing),
        base::Bind(&AliveChecker::OnResume, weak_factory_.GetWeakPtr()));
}

AliveChecker::~AliveChecker() {
  DCHECK(task_runner_->BelongsToCurrentThread());
}

void AliveChecker::Start() {
  DCHECK(task_runner_->BelongsToCurrentThread());
  SetLastAliveNotificationTimeToNowOnTaskRunner();
  DCHECK(!check_alive_timer_);
  check_alive_timer_ = std::make_unique<base::RepeatingTimer>();
  check_alive_timer_->Start(FROM_HERE, check_interval_, this,
                            &AliveChecker::CheckIfAlive);
  DCHECK(check_alive_timer_->IsRunning());
}

void AliveChecker::Stop() {
  DCHECK(task_runner_->BelongsToCurrentThread());
  check_alive_timer_.reset();
}

bool AliveChecker::DetectedDead() {
  DCHECK(task_runner_->BelongsToCurrentThread());
  return dead_detected_;
}

void AliveChecker::AliveNotification() {
  // We don't need high precision for setting |last_alive_notification_time_| so
  // we don't have to care about the delay added with posting the task.
  task_runner_->PostTask(
      FROM_HERE, base::BindOnce(&AliveChecker::AliveNotificationOnTaskRunner,
                                weak_factory_.GetWeakPtr()));
}

void AliveChecker::CheckIfAlive() {
  DCHECK(task_runner_->BelongsToCurrentThread());

  // The reason we check a flag instead of stopping the timer that runs this
  // function at suspend is that it would require knowing what state we're in
  // when resuming and maybe start the timer. Also, we would still need this
  // flag anyway to maybe start the timer at stream creation.
  if (power_observer_ && power_observer_->IsSuspending())
    return;

  if (base::TimeTicks::Now() - last_alive_notification_time_ > timeout_) {
    // TODO: Comment why post, or just run callback.
    task_runner_->PostTask(FROM_HERE, dead_callback_);
    dead_detected_ = true;
  }
}

void AliveChecker::AliveNotificationOnTaskRunner() {
  DCHECK(task_runner_->BelongsToCurrentThread());
  SetLastAliveNotificationTimeToNowOnTaskRunner();
  if (stop_at_first_alive_notification_)
    Stop();
}

void AliveChecker::SetLastAliveNotificationTimeToNowOnTaskRunner() {
  DCHECK(task_runner_->BelongsToCurrentThread());
  last_alive_notification_time_ = base::TimeTicks::Now();
}

void AliveChecker::OnResume() {
  DCHECK(task_runner_->BelongsToCurrentThread());
  SetLastAliveNotificationTimeToNowOnTaskRunner();
}

}  // namespace media
