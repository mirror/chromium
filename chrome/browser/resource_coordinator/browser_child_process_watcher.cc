// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/resource_coordinator/browser_child_process_watcher.h"

#include "base/process/process.h"
#include "content/public/browser/child_process_data.h"
#include "content/public/common/process_type.h"
#include "content/public/common/service_manager_connection.h"
#include "services/resource_coordinator/public/cpp/resource_coordinator_features.h"

namespace resource_coordinator {

BrowserChildProcessWatcher::BrowserChildProcessWatcher() {
  BrowserChildProcessObserver::Add(this);
}

BrowserChildProcessWatcher::~BrowserChildProcessWatcher() {
  BrowserChildProcessObserver::Remove(this);
}

void BrowserChildProcessWatcher::BrowserChildProcessLaunchedAndConnected(
    const content::ChildProcessData& data) {
  if (!resource_coordinator::IsResourceCoordinatorEnabled())
    return;

  if (data.process_type == content::PROCESS_TYPE_GPU) {
    DCHECK(!gpu_process_resource_coordinator_.get());
    gpu_process_resource_coordinator_ =
        base::MakeUnique<resource_coordinator::ResourceCoordinatorInterface>(
            content::ServiceManagerConnection::GetForProcess()->GetConnector(),
            resource_coordinator::CoordinationUnitType::kProcess);

    base::ProcessId pid = base::GetProcId(data.handle);
    gpu_process_resource_coordinator_->SetProperty(
        resource_coordinator::mojom::PropertyType::kPID, pid);

    gpu_process_resource_coordinator_->SetProperty(
        resource_coordinator::mojom::PropertyType::kLaunchTime,
        base::Time::Now().ToTimeT());
  }
}

}  // namespace resource_coordinator
