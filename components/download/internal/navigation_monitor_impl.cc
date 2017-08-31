// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/download/internal/navigation_monitor_impl.h"

#include "base/bind.h"
#include "base/callback_helpers.h"
#include "base/threading/thread_task_runner_handle.h"

namespace download {

NavigationMonitorImpl::NavigationMonitorImpl()
    : observer_(nullptr),
      current_navigation_count_(0),
      weak_ptr_factory_(this) {}

NavigationMonitorImpl::~NavigationMonitorImpl() = default;

void NavigationMonitorImpl::Configure(
    base::TimeDelta navigation_completion_delay,
    base::TimeDelta navigation_timeout_delay) {
  navigation_completion_delay_ = navigation_completion_delay;
  navigation_timeout_delay_ = navigation_timeout_delay * 5;
}

bool NavigationMonitorImpl::IsNavigationInProgress() {
  return current_navigation_count_ > 0;
}

void NavigationMonitorImpl::Start(NavigationMonitor::Observer* observer) {
  DCHECK(!observer_) << "Only set observer for once.";
  observer_ = observer;
}

void NavigationMonitorImpl::OnNavigationEvent(NavigationEvent event) {
  if (event == NavigationEvent::START) {
    current_navigation_count_++;
    if (observer_)
      observer_->OnNavigationEvent(event);

    navigation_finished_callback_.Cancel();
    ScheduleBackupTask();
  } else {
    if (current_navigation_count_ == 0) {
      // Somehow we didn't record the beginning of the navigation. No need to
      // take any action.
      backup_navigation_finished_callback_.Cancel();
      navigation_finished_callback_.Cancel();
      return;
    }

    current_navigation_count_--;
    if (current_navigation_count_ == 0)
      NotifyNavigationFinished();
  }
}

void NavigationMonitorImpl::NotifyNavigationFinished() {
  backup_navigation_finished_callback_.Cancel();
  navigation_finished_callback_.Reset(
      base::Bind(&NavigationMonitorImpl::OnNavigationFinished,
                 weak_ptr_factory_.GetWeakPtr()));
  base::ThreadTaskRunnerHandle::Get()->PostDelayedTask(
      FROM_HERE, navigation_finished_callback_.callback(),
      navigation_completion_delay_);
}

void NavigationMonitorImpl::OnNavigationFinished() {
  if (observer_)
    observer_->OnNavigationEvent(NavigationEvent::END);
}

void NavigationMonitorImpl::ScheduleBackupTask() {
  backup_navigation_finished_callback_.Reset(
      base::Bind(&NavigationMonitorImpl::OnNavigationFinished,
                 weak_ptr_factory_.GetWeakPtr()));
  base::ThreadTaskRunnerHandle::Get()->PostDelayedTask(
      FROM_HERE, backup_navigation_finished_callback_.callback(),
      navigation_timeout_delay_);
}

}  // namespace download
