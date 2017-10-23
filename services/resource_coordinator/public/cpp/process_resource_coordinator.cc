// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/resource_coordinator/public/cpp/process_resource_coordinator.h"

#include "services/resource_coordinator/public/interfaces/coordination_unit_provider.mojom.h"
#include "services/resource_coordinator/public/interfaces/service_constants.mojom.h"
#include "services/service_manager/public/cpp/connector.h"

namespace resource_coordinator {

namespace {

void OnConnectionError() {
  DCHECK(false);
}

}  // namespace

ProcessResourceCoordinator::ProcessResourceCoordinator(
    service_manager::Connector* connector)
    : weak_ptr_factory_(this) {
  CoordinationUnitID new_cu_id(CoordinationUnitType::kProcess, std::string());
  ConnectToService(connector, new_cu_id);
}

ProcessResourceCoordinator::~ProcessResourceCoordinator() = default;

void ProcessResourceCoordinator::AddBinding(
    mojom::ProcessCoordinationUnitRequest request) {
  if (!service_)
    return;
  service_->AddBinding(std::move(request));
}

void ProcessResourceCoordinator::SetCPUUsage(double cpu_usage) {
  if (!service_)
    return;
  service_->SetCPUUsage(cpu_usage);
}

void ProcessResourceCoordinator::SetLaunchTime(base::Time launch_time) {
  if (!service_)
    return;
  service_->SetLaunchTime(launch_time);
}

void ProcessResourceCoordinator::SetPID(int64_t pid) {
  if (!service_)
    return;
  service_->SetPID(pid);
}

void ProcessResourceCoordinator::AddFrame(
    const FrameResourceCoordinator& frame) {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  if (!service_)
    return;
  // We could keep the ID around ourselves, but this hop ensures that the child
  // has been created on the service-side.
  frame.service()->GetID(base::Bind(&ProcessResourceCoordinator::AddFrameByID,
                                    weak_ptr_factory_.GetWeakPtr()));
}

void ProcessResourceCoordinator::RemoveFrame(
    const FrameResourceCoordinator& frame) {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  if (!service_)
    return;
  frame.service()->GetID(
      base::Bind(&ProcessResourceCoordinator::RemoveFrameByID,
                 weak_ptr_factory_.GetWeakPtr()));
}

void ProcessResourceCoordinator::ConnectToService(
    service_manager::Connector* connector,
    const CoordinationUnitID& cu_id) {
  if (!connector)
    return;
  cu_id_ = cu_id;
  mojom::CoordinationUnitProviderPtr provider;
  connector->BindInterface(mojom::kServiceName, mojo::MakeRequest(&provider));
  provider->CreateProcessCoordinationUnit(mojo::MakeRequest(&service_), cu_id);
  service_.set_connection_error_handler(base::Bind(&OnConnectionError));
}

void ProcessResourceCoordinator::AddFrameByID(const CoordinationUnitID& cu_id) {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  service_->AddFrame(cu_id);
}

void ProcessResourceCoordinator::RemoveFrameByID(
    const CoordinationUnitID& cu_id) {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  service_->RemoveFrame(cu_id);
}

}  // namespace resource_coordinator
