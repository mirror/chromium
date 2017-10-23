// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/resource_coordinator/public/cpp/frame_resource_coordinator.h"

#include "services/resource_coordinator/public/interfaces/coordination_unit_provider.mojom.h"
#include "services/resource_coordinator/public/interfaces/service_constants.mojom.h"
#include "services/service_manager/public/cpp/connector.h"

namespace resource_coordinator {

namespace {

void OnConnectionError() {
  DCHECK(false);
}

}  // namespace

FrameResourceCoordinator::FrameResourceCoordinator(
    service_manager::Connector* connector)
    : weak_ptr_factory_(this) {
  CoordinationUnitID new_cu_id(CoordinationUnitType::kFrame, std::string());
  ConnectToService(connector, new_cu_id);
}

FrameResourceCoordinator::~FrameResourceCoordinator() = default;

void FrameResourceCoordinator::AddBinding(
    mojom::FrameCoordinationUnitRequest request) {
  if (!service_)
    return;
  service_->AddBinding(std::move(request));
}

void FrameResourceCoordinator::SetAudibility(bool audible) {
  if (!service_)
    return;
  service_->SetAudibility(audible);
}

void FrameResourceCoordinator::OnAlertFired() {
  if (!service_)
    return;
  service_->OnAlertFired();
}

void FrameResourceCoordinator::AddChildFrame(
    const FrameResourceCoordinator& child) {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  if (!service_)
    return;
  // We could keep the ID around ourselves, but this hop ensures that the child
  // has been created on the service-side.
  child.service()->GetID(
      base::Bind(&FrameResourceCoordinator::AddChildFrameByID,
                 weak_ptr_factory_.GetWeakPtr()));
}

void FrameResourceCoordinator::RemoveChildFrame(
    const FrameResourceCoordinator& child) {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  if (!service_)
    return;
  child.service()->GetID(
      base::Bind(&FrameResourceCoordinator::RemoveChildFrameByID,
                 weak_ptr_factory_.GetWeakPtr()));
}

void FrameResourceCoordinator::ConnectToService(
    service_manager::Connector* connector,
    const CoordinationUnitID& cu_id) {
  if (!connector)
    return;
  cu_id_ = cu_id;
  mojom::CoordinationUnitProviderPtr provider;
  connector->BindInterface(mojom::kServiceName, mojo::MakeRequest(&provider));
  provider->CreateFrameCoordinationUnit(mojo::MakeRequest(&service_), cu_id);
  service_.set_connection_error_handler(base::Bind(&OnConnectionError));
}

void FrameResourceCoordinator::AddChildFrameByID(
    const CoordinationUnitID& child_id) {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  service_->AddChildFrame(child_id);
}

void FrameResourceCoordinator::RemoveChildFrameByID(
    const CoordinationUnitID& child_id) {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  service_->RemoveChildFrame(child_id);
}

}  // namespace resource_coordinator
