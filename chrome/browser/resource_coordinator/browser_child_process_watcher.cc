// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/resource_coordinator/browser_child_process_watcher.h"

#include "base/process/process.h"
#include "content/public/browser/child_process_data.h"
#include "content/public/common/service_manager_connection.h"
#include "services/resource_coordinator/public/cpp/resource_coordinator_features.h"

namespace resource_coordinator {

BrowserChildProcessWatcher::BrowserChildProcessWatcher(
    resource_coordinator::ResourceCoordinatorInterface*
        process_resource_coordinator)
    : process_resource_coordinator_(process_resource_coordinator) {
  DCHECK(process_resource_coordinator);
  BrowserChildProcessObserver::Add(this);
}

BrowserChildProcessWatcher::~BrowserChildProcessWatcher() {
  BrowserChildProcessObserver::Remove(this);
}

void BrowserChildProcessWatcher::BrowserChildProcessLaunchedAndConnected(
    const content::ChildProcessData& data) {
  if (!resource_coordinator::IsResourceCoordinatorEnabled())
    return;

  base::ProcessId pid = base::GetProcId(data.handle);
  process_resource_coordinator_->SetProperty(
      resource_coordinator::mojom::PropertyType::kPID, pid);

  process_resource_coordinator_->SetProperty(
      resource_coordinator::mojom::PropertyType::kLaunchTime,
      base::Time::Now().ToTimeT());
}

}  // namespace resource_coordinator
