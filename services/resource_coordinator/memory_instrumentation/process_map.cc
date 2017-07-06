// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/resource_coordinator/memory_instrumentation/process_map.h"

#include "base/process/process_handle.h"
#include "mojo/public/cpp/bindings/binding.h"
#include "services/resource_coordinator/public/interfaces/memory_instrumentation/memory_instrumentation.mojom.h"
#include "services/service_manager/public/cpp/connector.h"
#include "services/service_manager/public/cpp/identity.h"
#include "services/service_manager/public/interfaces/constants.mojom.h"
#include "services/service_manager/public/interfaces/service_manager.mojom.h"

namespace memory_instrumentation {

ProcessMap::ProcessMap(service_manager::Connector* connector) : binding_(this) {
  if (!connector)
    return;  // Happens in unittests.
  service_manager::mojom::ServiceManagerPtr service_manager;
  connector->BindInterface(service_manager::mojom::kServiceName,
                           &service_manager);
  service_manager::mojom::ServiceManagerListenerPtr listener;
  service_manager::mojom::ServiceManagerListenerRequest request(
      mojo::MakeRequest(&listener));
  service_manager->AddListener(std::move(listener));
  binding_.Bind(std::move(request));
}

ProcessMap::~ProcessMap() {}

void ProcessMap::OnInit(std::vector<RunningServiceInfoPtr> instances) {
  LOG(ERROR) << "OnInit";
  for (RunningServiceInfoPtr& instance : instances) {
    OnServiceCreated(std::move(instance));
  }
  LOG(ERROR) << "OnInit Done";
}

void ProcessMap::OnServiceCreated(RunningServiceInfoPtr instance) {
  LOG(ERROR) << "OnServiceCreated " << instance->pid << " " << instance->identity.name() << "." << instance->identity.instance();
  if (instance->pid == base::kNullProcessId)
    return;
  const service_manager::Identity& identity = instance->identity;
  DCHECK_EQ(0u, instances_.count(identity));
  instances_.emplace(identity, instance->pid);
}

void ProcessMap::OnServiceStarted(const service_manager::Identity& identity,
                                  uint32_t pid) {
  LOG(ERROR) << "OnServiceStarted " << pid << " " << identity.name() << "." << identity.instance();
  if (pid == base::kNullProcessId)
    return;
  instances_[identity] = pid;
}

void ProcessMap::OnServiceFailedToStart(const service_manager::Identity& identity) {
  LOG(ERROR) << "OnServiceFailedToStart" << identity.name() << "." << identity.instance();
}

void ProcessMap::OnServiceStopped(const service_manager::Identity& identity) {
  LOG(ERROR) << "OnServiceStopped" << identity.name() << "." << identity.instance();
  instances_.erase(identity);
}

base::ProcessId ProcessMap::GetProcessId(
    const service_manager::Identity& identity) const {
  LOG(ERROR) << "GetProcessId " << identity.name() << "." << identity.instance();
  auto it = instances_.find(identity);
  return it != instances_.end() ? it->second : base::kNullProcessId;
}

}  // namespace memory_instrumentation
