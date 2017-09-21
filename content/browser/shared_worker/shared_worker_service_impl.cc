// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/shared_worker/shared_worker_service_impl.h"

#include <stddef.h>

#include <algorithm>
#include <iterator>

#include "base/callback.h"
#include "base/logging.h"
#include "base/macros.h"
#include "base/memory/ptr_util.h"
#include "content/browser/devtools/shared_worker_devtools_manager.h"
#include "content/browser/renderer_host/render_process_host_impl.h"
#include "content/browser/shared_worker/shared_worker_host.h"
#include "content/browser/shared_worker/shared_worker_instance.h"
#include "content/common/message_port.h"
#include "content/common/shared_worker/shared_worker_client.mojom.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/worker_service_observer.h"
#include "content/public/common/bind_interface_helpers.h"

namespace content {
namespace {

bool IsProcessAliveHelper(RenderProcessHostImpl* host) {
  return host && !host->FastShutdownStarted() &&
         !host->IsKeepAliveRefCountDisabled();
}

bool IsProcessAlive(int process_id) {
  auto* host = static_cast<RenderProcessHostImpl*>(
      RenderProcessHost::FromID(process_id));
  return IsProcessAliveHelper(host);
}

bool ReserveProcess(int process_id,
                    int* route_id,
                    mojom::SharedWorkerFactoryPtr* factory) {
  auto* host = static_cast<RenderProcessHostImpl*>(
      RenderProcessHost::FromID(process_id));
  if (!IsProcessAliveHelper(host))
    return false;
  host->IncrementKeepAliveRefCount();

  *route_id = host->GetNextRoutingID();
  BindInterface(host, factory);
  return true;
}

void ReleaseProcess(int process_id) {
  auto* host = static_cast<RenderProcessHostImpl*>(
      RenderProcessHost::FromID(process_id));
  if (IsProcessAliveHelper(host))
    host->DecrementKeepAliveRefCount();
}

}  // namespace

WorkerService* WorkerService::GetInstance() {
  return SharedWorkerServiceImpl::GetInstance();
}

// static
SharedWorkerServiceImpl::ReserveProcessFunc
    SharedWorkerServiceImpl::s_reserve_process_func_ = ReserveProcess;
SharedWorkerServiceImpl::ReleaseProcessFunc
    SharedWorkerServiceImpl::s_release_process_func_ = ReleaseProcess;
SharedWorkerServiceImpl::IsProcessAliveFunc
    SharedWorkerServiceImpl::s_is_process_alive_func_ = IsProcessAlive;

SharedWorkerServiceImpl* SharedWorkerServiceImpl::GetInstance() {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  return base::Singleton<SharedWorkerServiceImpl>::get();
}

SharedWorkerServiceImpl::SharedWorkerServiceImpl() {}

SharedWorkerServiceImpl::~SharedWorkerServiceImpl() {}

void SharedWorkerServiceImpl::ResetForTesting() {
  worker_hosts_.clear();
  observers_.Clear();
  s_reserve_process_func_ = ReserveProcess;
  s_release_process_func_ = ReleaseProcess;
}

bool SharedWorkerServiceImpl::TerminateWorker(int process_id, int route_id) {
  SharedWorkerHost* host = FindSharedWorkerHost(process_id, route_id);
  if (!host || !host->instance())
    return false;
  host->TerminateWorker();
  return true;
}

std::vector<WorkerService::WorkerInfo> SharedWorkerServiceImpl::GetWorkers() {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  std::vector<WorkerService::WorkerInfo> results;
  for (const auto& iter : worker_hosts_) {
    SharedWorkerHost* host = iter.second.get();
    const SharedWorkerInstance* instance = host->instance();
    if (instance) {
      WorkerService::WorkerInfo info;
      info.url = instance->url();
      info.name = instance->name();
      info.route_id = host->route_id();
      info.process_id = host->process_id();
      results.push_back(info);
    }
  }
  return results;
}

void SharedWorkerServiceImpl::AddObserver(WorkerServiceObserver* observer) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  observers_.AddObserver(observer);
}

void SharedWorkerServiceImpl::RemoveObserver(WorkerServiceObserver* observer) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  observers_.RemoveObserver(observer);
}

void SharedWorkerServiceImpl::ConnectToWorker(
    int process_id,
    int frame_id,
    mojom::SharedWorkerInfoPtr info,
    mojom::SharedWorkerClientPtr client,
    blink::mojom::SharedWorkerCreationContextType creation_context_type,
    const MessagePort& message_port,
    ResourceContext* resource_context,
    const WorkerStoragePartitionId& partition_id) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);

  auto instance = std::make_unique<SharedWorkerInstance>(
      info->url, info->name, info->content_security_policy,
      info->content_security_policy_type, info->creation_address_space,
      resource_context, partition_id, creation_context_type,
      info->data_saver_enabled);

  SharedWorkerHost* host = FindSharedWorkerHost(*instance);
  if (host) {
    if (s_is_process_alive_func_(host->process_id())) {
      host->AddClient(std::move(client), process_id, frame_id, message_port);
      return;
    }
    DestroyHost(host->process_id(), host->route_id());
  }

  CreateWorker(std::move(instance), std::move(client), process_id, frame_id,
               message_port);
}

void SharedWorkerServiceImpl::DestroyHost(int process_id, int route_id) {
  worker_hosts_.erase(WorkerID(process_id, route_id));

  for (auto& observer : observers_)
    observer.WorkerDestroyed(process_id, route_id);

  s_release_process_func_(process_id);
}

void SharedWorkerServiceImpl::CreateWorker(
    std::unique_ptr<SharedWorkerInstance> instance,
    mojom::SharedWorkerClientPtr client,
    int process_id,
    int frame_id,
    const MessagePort& message_port) {
  // Re-use the process that requested the shared worker.
  int worker_process_id = process_id;

  int worker_route_id;
  mojom::SharedWorkerFactoryPtr factory;
  if (!s_reserve_process_func_(worker_process_id, &worker_route_id, &factory)) {
    // The requesting process must be shutting down, so just drop this request.
    return;
  }
  bool pause_on_start =
      SharedWorkerDevToolsManager::GetInstance()->WorkerCreated(
          worker_process_id, worker_route_id, *instance);

  auto host = std::make_unique<SharedWorkerHost>(
      std::move(instance), worker_process_id, worker_route_id);
  host->Start(std::move(factory), pause_on_start);
  host->AddClient(std::move(client), process_id, frame_id, message_port);

  const GURL url = host->instance()->url();
  const std::string name = host->instance()->name();

  worker_hosts_[WorkerID(worker_process_id, worker_route_id)] = std::move(host);

  for (auto& observer : observers_)
    observer.WorkerCreated(url, name, worker_process_id, worker_route_id);
}

SharedWorkerHost* SharedWorkerServiceImpl::FindSharedWorkerHost(int process_id,
                                                                int route_id) {
  auto iter = worker_hosts_.find(WorkerID(process_id, route_id));
  if (iter == worker_hosts_.end())
    return nullptr;
  return iter->second.get();
}

SharedWorkerHost* SharedWorkerServiceImpl::FindSharedWorkerHost(
    const SharedWorkerInstance& instance) {
  for (const auto& iter : worker_hosts_) {
    SharedWorkerHost* host = iter.second.get();
    if (host->IsAvailable() && host->instance()->Matches(instance))
      return host;
  }
  return nullptr;
}

}  // namespace content
