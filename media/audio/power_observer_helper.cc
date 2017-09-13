// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "media/audio/power_observer_helper.h"

#include <utility>

#include "base/bind.h"
#include "base/power_monitor/power_monitor.h"

namespace media {

PowerObserverHelper::PowerObserverHelper(
    const scoped_refptr<base::SequencedTaskRunner>& task_runner,
    const base::RepeatingClosure& suspend_callback,
    const base::RepeatingClosure& resume_callback)
    : task_runner_(task_runner),
      suspend_callback_(std::move(suspend_callback)),
      resume_callback_(std::move(resume_callback)),
      weak_factory_(this) {
  // The PowerMonitor requires significant setup (a CFRunLoop and preallocated
  // IO ports) so it's not available under unit tests.  See the OSX impl of
  // base::PowerMonitorDeviceSource for more details.
  // TODO(grunell): We could be suspending when adding this as observer, and
  // we won't be notified about that. See if we can add
  // PowerMonitorSource::IsSuspending() so that this can be checked here.
  if (auto* power_monitor = base::PowerMonitor::Get())
    power_monitor->AddObserver(this);
}

PowerObserverHelper::~PowerObserverHelper() {
  if (auto* power_monitor = base::PowerMonitor::Get())
    power_monitor->RemoveObserver(this);
}

bool PowerObserverHelper::IsSuspending() const {
  DCHECK(task_runner_->RunsTasksInCurrentSequence());
  return is_suspending_;
}

void PowerObserverHelper::OnSuspend() {
  DVLOG(1) << "OnSuspend";
  if (task_runner_->RunsTasksInCurrentSequence()) {
    OnSuspendOnTaskRunner();
  } else {
    task_runner_->PostTask(
        FROM_HERE, base::BindOnce(&PowerObserverHelper::OnSuspendOnTaskRunner,
                                  weak_factory_.GetWeakPtr()));
  }
}

void PowerObserverHelper::OnResume() {
  DVLOG(1) << "OnResume";
  if (task_runner_->RunsTasksInCurrentSequence()) {
    OnResumeOnTaskRunner();
  } else {
    task_runner_->PostTask(
        FROM_HERE, base::BindOnce(&PowerObserverHelper::OnResumeOnTaskRunner,
                                  weak_factory_.GetWeakPtr()));
  }
}

void PowerObserverHelper::OnSuspendOnTaskRunner() {
  DCHECK(task_runner_->RunsTasksInCurrentSequence());
  is_suspending_ = true;
  suspend_callback_.Run();
}

void PowerObserverHelper::OnResumeOnTaskRunner() {
  DCHECK(task_runner_->RunsTasksInCurrentSequence());
  is_suspending_ = false;
  resume_callback_.Run();
}

}  // namespace media
