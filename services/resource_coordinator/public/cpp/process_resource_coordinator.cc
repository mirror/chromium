// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/resource_coordinator/public/cpp/process_resource_coordinator.h"

namespace resource_coordinator {

ProcessResourceCoordinator::ProcessResourceCoordinator(
    service_manager::Connector* connector)
    : ResourceCoordinatorInterface<mojom::ProcessCoordinationUnitPtr,
                                   mojom::ProcessCoordinationUnitRequest>(
          connector,
          CoordinationUnitType::kProcess),
      weak_ptr_factory_(this) {}

ProcessResourceCoordinator::~ProcessResourceCoordinator() = default;

void ProcessResourceCoordinator::SetCPUUsage(double cpu_usage) {
  if (!service_)
    return;
  service_->SetCPUUsage(cpu_usage * 1000);
}

void ProcessResourceCoordinator::SetExpectedTaskQueueingDuration(
    double duration) {
  if (!service_)
    return;
  service_->SetExpectedTaskQueueingDuration(duration);
}

void ProcessResourceCoordinator::SetPID(int64_t pid) {
  if (!service_)
    return;
  service_->SetPID(pid);
}

void ProcessResourceCoordinator::AddFrame(
    const FrameResourceCoordinator& frame) {
  DCHECK(thread_checker_.CalledOnValidThread());
  if (!service_)
    return;
  // We could keep the ID around ourselves, but this hop ensures that the child
  // has been created on the service-side.
  frame.service()->GetID(base::Bind(&ProcessResourceCoordinator::AddFrameByID,
                                    weak_ptr_factory_.GetWeakPtr()));
}

void ProcessResourceCoordinator::RemoveFrame(
    const FrameResourceCoordinator& frame) {
  DCHECK(thread_checker_.CalledOnValidThread());
  if (!service_)
    return;
  frame.service()->GetID(
      base::Bind(&ProcessResourceCoordinator::RemoveFrameByID,
                 weak_ptr_factory_.GetWeakPtr()));
}

void ProcessResourceCoordinator::ConnectToService(
    mojom::CoordinationUnitProviderPtr& provider,
    const CoordinationUnitID& cu_id) {
  provider->CreateProcessCoordinationUnit(mojo::MakeRequest(&service_), cu_id);
}

void ProcessResourceCoordinator::AddFrameByID(const CoordinationUnitID& cu_id) {
  DCHECK(thread_checker_.CalledOnValidThread());
  service_->AddFrame(cu_id);
}

void ProcessResourceCoordinator::RemoveFrameByID(
    const CoordinationUnitID& cu_id) {
  DCHECK(thread_checker_.CalledOnValidThread());
  service_->RemoveFrame(cu_id);
}

}  // namespace resource_coordinator
