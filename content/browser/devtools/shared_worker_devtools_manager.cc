// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/devtools/shared_worker_devtools_manager.h"

#include "content/browser/devtools/shared_worker_devtools_agent_host.h"
#include "content/browser/shared_worker/shared_worker_instance.h"
#include "content/public/browser/browser_thread.h"

namespace content {

// static
SharedWorkerDevToolsManager* SharedWorkerDevToolsManager::GetInstance() {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  return base::Singleton<SharedWorkerDevToolsManager>::get();
}

DevToolsAgentHostImpl*
SharedWorkerDevToolsManager::GetDevToolsAgentHostForWorker(
    int worker_process_id,
    int worker_route_id) {
  AgentHostMap::iterator it = workers_.find(
      WorkerId(worker_process_id, worker_route_id));
  return it == workers_.end() ? NULL : it->second;
}

void SharedWorkerDevToolsManager::AddAllAgentHosts(
    SharedWorkerDevToolsAgentHost::List* result) {
  for (auto& worker : workers_) {
    if (!worker.second->IsTerminated())
      result->push_back(worker.second);
  }
}

void SharedWorkerDevToolsManager::WorkerCreated(
    int worker_process_id,
    int worker_route_id,
    const SharedWorkerInstance& instance,
    bool* pause_on_start,
    base::UnguessableToken* devtools_worker_token) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  const WorkerId id(worker_process_id, worker_route_id);
  AgentHostMap::iterator it =
      FindExistingWorkerAgentHost(instance);
  if (it == workers_.end()) {
    *pause_on_start = false;
    *devtools_worker_token = base::UnguessableToken::Create();
    workers_[id] =
        new SharedWorkerDevToolsAgentHost(*devtools_worker_token, id, instance);
    return;
  }

  // Worker restarted.
  SharedWorkerDevToolsAgentHost* agent_host = it->second;
  agent_host->WorkerRestarted(id);
  workers_.erase(it);
  workers_[id] = agent_host;
  *pause_on_start = agent_host->IsAttached();
  *devtools_worker_token = workers_[id]->devtools_worker_token();
}

void SharedWorkerDevToolsManager::WorkerReadyForInspection(
    int worker_process_id,
    int worker_route_id) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  const WorkerId id(worker_process_id, worker_route_id);
  AgentHostMap::iterator it = workers_.find(id);
  if (it == workers_.end() || it->second->IsTerminated())
    return;
  it->second->WorkerReadyForInspection();
}

void SharedWorkerDevToolsManager::WorkerDestroyed(
    int worker_process_id,
    int worker_route_id) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  const WorkerId id(worker_process_id, worker_route_id);
  AgentHostMap::iterator it = workers_.find(id);
  if (it == workers_.end() || it->second->IsTerminated())
    return;
  scoped_refptr<SharedWorkerDevToolsAgentHost> agent_host(it->second);
  agent_host->WorkerDestroyed();
}

void SharedWorkerDevToolsManager::RemoveInspectedWorkerData(WorkerId id) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  workers_.erase(id);
}
SharedWorkerDevToolsManager::SharedWorkerDevToolsManager() {
}

SharedWorkerDevToolsManager::~SharedWorkerDevToolsManager() {
}

SharedWorkerDevToolsManager::AgentHostMap::iterator
SharedWorkerDevToolsManager::FindExistingWorkerAgentHost(
    const SharedWorkerInstance& instance) {
  AgentHostMap::iterator it = workers_.begin();
  for (; it != workers_.end(); ++it) {
    if (it->second->Matches(instance))
      break;
  }
  return it;
}

void SharedWorkerDevToolsManager::ResetForTesting() {
  workers_.clear();
}


}  // namespace content
