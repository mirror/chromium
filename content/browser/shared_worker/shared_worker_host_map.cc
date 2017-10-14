// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/shared_worker/shared_worker_host_map.h"

#include "content/browser/shared_worker/shared_worker_host.h"
#include "content/browser/shared_worker/shared_worker_instance.h"
#include "content/browser/shared_worker/shared_worker_service_impl.h"

namespace content {

SharedWorkerHostMap::SharedWorkerHostMap() {
  SharedWorkerServiceImpl::GetInstance()->RegisterHostMap(this);
}

SharedWorkerHostMap::~SharedWorkerHostMap() {
  SharedWorkerServiceImpl::GetInstance()->UnregisterHostMap(this);
}

void SharedWorkerHostMap::Add(SharedWorkerID id,
                              std::unique_ptr<SharedWorkerHost> host) {
  hosts_[id] = std::move(host);
}

void SharedWorkerHostMap::Remove(SharedWorkerID id) {
  hosts_.erase(id);

  // Complete the call to TerminateAllWorkersForTesting if no more workers.
  if (hosts_.empty() && terminate_all_workers_callback_) {
    base::ThreadTaskRunnerHandle::Get()->PostTask(
        FROM_HERE, std::move(terminate_all_workers_callback_));
  }
}

SharedWorkerHost* SharedWorkerHostMap::Find(SharedWorkerID id) {
  auto iter = hosts_.find(id);
  if (iter == hosts_.end())
    return nullptr;
  return iter->second.get();
}

SharedWorkerHost* SharedWorkerHostMap::FindAvailable(
    const SharedWorkerInstance& instance) {
  for (const auto& iter : hosts_) {
    SharedWorkerHost* host = iter.second.get();
    if (host->IsAvailable() && host->instance()->Matches(instance))
      return host;
  }
  return nullptr;
}

void SharedWorkerHostMap::GetWorkers(
    std::vector<WorkerService::WorkerInfo>* results) {
  for (const auto& iter : hosts_) {
    const SharedWorkerHost* host = iter.second.get();
    const SharedWorkerInstance* instance = host->instance();
    WorkerService::WorkerInfo info;
    info.url = instance->url();
    info.name = instance->name();
    info.route_id = host->route_id();
    info.process_id = host->process_id();
    results->push_back(info);
  }
}

void SharedWorkerHostMap::TerminateAllWorkersForTesting(
    base::OnceClosure callback) {
  DCHECK(!terminate_all_workers_callback_);
  if (hosts_.empty()) {
    // Run callback asynchronously to avoid re-entering the caller.
    base::ThreadTaskRunnerHandle::Get()->PostTask(FROM_HERE,
                                                  std::move(callback));
  } else {
    terminate_all_workers_callback_ = std::move(callback);
    for (auto& iter : hosts_)
      iter.second->TerminateWorker();
    // Monitor for actual termination in Remove.
  }
}

void SharedWorkerHostMap::ResetForTesting() {
  hosts_.clear();
}

}  // namespace content
