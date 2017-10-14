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
#include "content/browser/shared_worker/shared_worker_host.h"
#include "content/browser/shared_worker/shared_worker_host_map.h"
#include "content/browser/shared_worker/shared_worker_instance.h"
#include "content/browser/storage_partition_impl.h"
#include "content/common/shared_worker/shared_worker_client.mojom.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/render_process_host.h"
#include "content/public/browser/worker_service_observer.h"
#include "content/public/common/bind_interface_helpers.h"
#include "third_party/WebKit/common/message_port/message_port_channel.h"

namespace content {
namespace {

bool IsShuttingDown(RenderProcessHost* host) {
  return !host || host->FastShutdownStarted() ||
         host->IsKeepAliveRefCountDisabled();
}

SharedWorkerHostMap* GetSharedWorkerHostMap(RenderProcessHost* host) {
  StoragePartition* storage_partition = host->GetStoragePartition();
  if (!storage_partition)
    return nullptr;
  return static_cast<StoragePartitionImpl*>(storage_partition)
      ->GetSharedWorkerHostMap();
}

}  // namespace

WorkerService* WorkerService::GetInstance() {
  return SharedWorkerServiceImpl::GetInstance();
}

SharedWorkerServiceImpl* SharedWorkerServiceImpl::GetInstance() {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  // OK to just leak the instance.
  static SharedWorkerServiceImpl* instance = nullptr;
  if (!instance)
    instance = new SharedWorkerServiceImpl();
  return instance;
}

bool SharedWorkerServiceImpl::TerminateWorker(int process_id, int route_id) {
  SharedWorkerHostMap* host_map =
      GetSharedWorkerHostMap(RenderProcessHost::FromID(process_id));
  if (!host_map)
    return false;

  SharedWorkerHost* host = host_map->Find(SharedWorkerID(process_id, route_id));
  if (!host)
    return false;

  host->TerminateWorker();
  return true;
}

void SharedWorkerServiceImpl::TerminateAllWorkersForTesting(
    StoragePartition* storage_partition,
    base::OnceClosure callback) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);

  SharedWorkerHostMap* host_map =
      static_cast<StoragePartitionImpl*>(storage_partition)
          ->GetSharedWorkerHostMap();
  host_map->TerminateAllWorkersForTesting(std::move(callback));
}

std::vector<WorkerService::WorkerInfo> SharedWorkerServiceImpl::GetWorkers() {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  std::vector<WorkerService::WorkerInfo> results;
  for (SharedWorkerHostMap* host_map : host_maps_)
    host_map->GetWorkers(&results);
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
    int from_process_id,
    int from_frame_id,
    mojom::SharedWorkerInfoPtr info,
    mojom::SharedWorkerClientPtr client,
    blink::mojom::SharedWorkerCreationContextType creation_context_type,
    const blink::MessagePortChannel& message_port) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);

  SharedWorkerHostMap* host_map =
      GetSharedWorkerHostMap(RenderProcessHost::FromID(from_process_id));
  if (!host_map)
    return;

  auto instance = std::make_unique<SharedWorkerInstance>(std::move(info),
                                                         creation_context_type);

  SharedWorkerHost* host = host_map->FindAvailable(*instance);
  if (host) {
    // The process may be shutting down, in which case we will try to create a
    // new shared worker instead.
    if (!IsShuttingDown(RenderProcessHost::FromID(host->process_id()))) {
      host->AddClient(std::move(client), from_process_id, from_frame_id,
                      message_port);
      return;
    }
    // Cleanup the existing shared worker now, to avoid having two matching
    // instances. This host would likely be observing the destruction of the
    // child process shortly, but we can clean this up now to avoid some
    // complexity.
    DestroyHost(host->process_id(), host->route_id());
  }

  CreateWorker(host_map, std::move(instance), std::move(client),
               from_process_id, from_frame_id, message_port);
}

void SharedWorkerServiceImpl::DestroyHost(int process_id, int route_id) {
  RenderProcessHost* process_host = RenderProcessHost::FromID(process_id);

  SharedWorkerHostMap* host_map = GetSharedWorkerHostMap(process_host);
  CHECK(host_map);

  host_map->Remove(SharedWorkerID(process_id, route_id));

  for (auto& observer : observers_)
    observer.WorkerDestroyed(process_id, route_id);

  if (!IsShuttingDown(process_host))
    process_host->DecrementKeepAliveRefCount();
}

void SharedWorkerServiceImpl::RegisterHostMap(SharedWorkerHostMap* host_map) {
  host_maps_.insert(host_map);
}

void SharedWorkerServiceImpl::UnregisterHostMap(SharedWorkerHostMap* host_map) {
  host_maps_.erase(host_map);
}

SharedWorkerServiceImpl::SharedWorkerServiceImpl() {}

SharedWorkerServiceImpl::~SharedWorkerServiceImpl() {}

void SharedWorkerServiceImpl::ResetForTesting() {
  for (SharedWorkerHostMap* host_map : host_maps_)
    host_map->ResetForTesting();
  observers_.Clear();
}

void SharedWorkerServiceImpl::CreateWorker(
    SharedWorkerHostMap* host_map,
    std::unique_ptr<SharedWorkerInstance> instance,
    mojom::SharedWorkerClientPtr client,
    int process_id,
    int frame_id,
    const blink::MessagePortChannel& message_port) {
  // Re-use the process that requested the shared worker.
  int worker_process_id = process_id;

  RenderProcessHost* process_host = RenderProcessHost::FromID(process_id);
  // If the requesting process is shutting down, then just drop this request on
  // the floor. The client is not going to be around much longer anyways.
  if (IsShuttingDown(process_host))
    return;

  // Keep the renderer process alive that will be hosting the shared worker.
  process_host->IncrementKeepAliveRefCount();

  // TODO(darin): Eliminate the need for shared workers to have routing IDs.
  // Dev Tools will need to be modified to use something else as an identifier.
  int worker_route_id = process_host->GetNextRoutingID();

  bool pause_on_start =
      SharedWorkerDevToolsManager::GetInstance()->WorkerCreated(
          worker_process_id, worker_route_id, *instance);

  auto host = std::make_unique<SharedWorkerHost>(
      std::move(instance), worker_process_id, worker_route_id);

  // Get the factory used to instantiate the new shared worker instance in
  // the target process.
  mojom::SharedWorkerFactoryPtr factory;
  BindInterface(process_host, &factory);

  host->Start(std::move(factory), pause_on_start);
  host->AddClient(std::move(client), process_id, frame_id, message_port);

  const GURL url = host->instance()->url();
  const std::string name = host->instance()->name();

  host_map->Add(SharedWorkerID(worker_process_id, worker_route_id),
                std::move(host));

  for (auto& observer : observers_)
    observer.WorkerCreated(url, name, worker_process_id, worker_route_id);
}

}  // namespace content
