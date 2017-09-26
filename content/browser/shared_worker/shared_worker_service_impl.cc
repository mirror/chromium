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

// NOTE: We assume this class will only live temporarily on the stack. It just
// holds a raw pointer to the associated RenderProcessHost and makes no attempt
// to worry about lifetime of that raw pointer.
class SharedWorkerServiceImpl::ProcessHelperImpl : public ProcessHelper {
 public:
  static std::unique_ptr<ProcessHelper> Create(int process_id) {
    return std::make_unique<ProcessHelperImpl>(process_id);
  }

  explicit ProcessHelperImpl(int process_id)
      : host_(static_cast<RenderProcessHostImpl*>(
            RenderProcessHost::FromID(process_id))) {}
  ~ProcessHelperImpl() override {}

  bool IsShuttingDown() override {
    return !host_ || host_->FastShutdownStarted() ||
           host_->IsKeepAliveRefCountDisabled();
  }

  void IncrementKeepAliveRefCount() override {
    DCHECK(host_);
    host_->IncrementKeepAliveRefCount();
  }

  void DecrementKeepAliveRefCount() override {
    DCHECK(host_);
    host_->DecrementKeepAliveRefCount();
  }

  void GetSharedWorkerFactory(mojom::SharedWorkerFactoryPtr* factory) override {
    DCHECK(host_);
    BindInterface(host_, factory);
  }

  int GetNextRoutingID() override {
    DCHECK(host_);
    return host_->GetNextRoutingID();
  }

 private:
  RenderProcessHostImpl* host_;
};

WorkerService* WorkerService::GetInstance() {
  return SharedWorkerServiceImpl::GetInstance();
}

// static
SharedWorkerServiceImpl::MakeProcessHelperFunc
    SharedWorkerServiceImpl::s_make_process_helper_func_ =
        SharedWorkerServiceImpl::ProcessHelperImpl::Create;

SharedWorkerServiceImpl* SharedWorkerServiceImpl::GetInstance() {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  return base::Singleton<SharedWorkerServiceImpl>::get();
}

SharedWorkerServiceImpl::SharedWorkerServiceImpl() {}

SharedWorkerServiceImpl::~SharedWorkerServiceImpl() {}

void SharedWorkerServiceImpl::ResetForTesting() {
  worker_hosts_.clear();
  observers_.Clear();
  s_make_process_helper_func_ =
      SharedWorkerServiceImpl::ProcessHelperImpl::Create;
}

bool SharedWorkerServiceImpl::TerminateWorker(int process_id, int route_id) {
  SharedWorkerHost* host = FindSharedWorkerHost(process_id, route_id);
  if (!host || !host->instance())
    return false;
  host->TerminateWorker();
  return true;
}

void SharedWorkerServiceImpl::TerminateAllWorkersForTesting(
    base::OnceClosure callback) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  DCHECK(!terminate_all_workers_callback_);
  if (worker_hosts_.empty()) {
    std::move(callback).Run();
  } else {
    terminate_all_workers_callback_ = std::move(callback);
    for (auto& iter : worker_hosts_)
      iter.second->TerminateWorker();
    // Monitor for actual termination in DestroyHost.
  }
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
    // The process may be shutting down, in which case we will try to create a
    // new shared worker instead.
    if (!MakeProcessHelper(host->process_id())->IsShuttingDown()) {
      host->AddClient(std::move(client), process_id, frame_id, message_port);
      return;
    }
    // Cleanup the existing shared worker now, to avoid having two matching
    // instances. This host would likely be observing the destruction of the
    // child process shortly, but we can clean this up now to avoid some
    // complexity.
    DestroyHost(host->process_id(), host->route_id());
  }

  CreateWorker(std::move(instance), std::move(client), process_id, frame_id,
               message_port);
}

void SharedWorkerServiceImpl::DestroyHost(int process_id, int route_id) {
  worker_hosts_.erase(WorkerID(process_id, route_id));

  for (auto& observer : observers_)
    observer.WorkerDestroyed(process_id, route_id);

  // Complete the call to TerminateAllWorkersForTesting if no more workers.
  if (worker_hosts_.empty() && terminate_all_workers_callback_)
    std::move(terminate_all_workers_callback_).Run();

  std::unique_ptr<ProcessHelper> process = MakeProcessHelper(process_id);
  if (!process->IsShuttingDown())
    process->DecrementKeepAliveRefCount();
}

void SharedWorkerServiceImpl::CreateWorker(
    std::unique_ptr<SharedWorkerInstance> instance,
    mojom::SharedWorkerClientPtr client,
    int process_id,
    int frame_id,
    const MessagePort& message_port) {
  // Re-use the process that requested the shared worker.
  int worker_process_id = process_id;

  std::unique_ptr<ProcessHelper> process = MakeProcessHelper(worker_process_id);
  // If the requesting process is shutting down, then just drop this request on
  // the floor. The client is going to be around much longer anyways.
  if (process->IsShuttingDown())
    return;

  // Keep the renderer process alive that will be hosting the shared worker.
  process->IncrementKeepAliveRefCount();

  // TODO(darin): Eliminate the need for shared workers to have routing IDs.
  // Dev Tools will need to be modified to use something else as an identifier.
  int worker_route_id = process->GetNextRoutingID();

  bool pause_on_start =
      SharedWorkerDevToolsManager::GetInstance()->WorkerCreated(
          worker_process_id, worker_route_id, *instance);

  auto host = std::make_unique<SharedWorkerHost>(
      std::move(instance), worker_process_id, worker_route_id);

  // Get the factory used to instantiate the new shared worker instance in
  // the target process
  mojom::SharedWorkerFactoryPtr factory;
  process->GetSharedWorkerFactory(&factory);

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
