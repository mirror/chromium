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
      navigation_in_progress_(false),
      weak_ptr_factory_(this) {}

NavigationMonitorImpl::~NavigationMonitorImpl() = default;

void NavigationMonitorImpl::Configure(
    base::TimeDelta navigation_completion_delay) {
  navigation_completion_delay_ = navigation_completion_delay;
}

void NavigationMonitorImpl::Start(NavigationMonitor::Observer* observer) {
  DCHECK(!observer_) << "Only set observer for once.";
  observer_ = observer;
}

void NavigationMonitorImpl::OnNavigationEvent(NavigationEvent event) {
  if (event == NavigationEvent::START) {
    navigation_in_progress_ = true;
    if (observer_)
      observer_->OnNavigationEvent(event);
  } else {
    if (!navigation_in_progress_) {
      // Somehow we didn't record the beginning of the navigation. No need to
      // take any action.
      return;
    }

    reset_navigation_callback_.Reset(
        base::Bind(&NavigationMonitorImpl::ResetNavigationFlag,
                   weak_ptr_factory_.GetWeakPtr()));
    base::ThreadTaskRunnerHandle::Get()->PostDelayedTask(
        FROM_HERE, reset_navigation_callback_.callback(),
        navigation_completion_delay_);
  }
}

void NavigationMonitorImpl::ResetNavigationFlag() {
  navigation_in_progress_ = false;
  if (observer_)
    observer_->OnNavigationEvent(NavigationEvent::END);
}

bool NavigationMonitorImpl::IsNavigationInProgress() {
  return navigation_in_progress_;
}

}  // namespace download
