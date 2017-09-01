// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/download/internal/navigation_monitor_impl.h"

#include "base/logging.h"

namespace download {

NavigationMonitorImpl::NavigationMonitorImpl() = default;

NavigationMonitorImpl::~NavigationMonitorImpl() = default;

void NavigationMonitorImpl::SetObserver(NavigationMonitor::Observer* observer) {
  observer_ = observer;
}

void NavigationMonitorImpl::OnNavigationEvent(NavigationEvent event) {
  if (observer_)
    observer_->OnNavigationEvent(event);
}

}  // namespace download
