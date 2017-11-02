// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/renderer/service_worker/controller_service_worker_connector.h"

#include "base/bind.h"
#include "base/bind_helpers.h"
#include "content/common/service_worker/service_worker_container.mojom.h"
#include "mojo/public/cpp/bindings/interface_request.h"

namespace content {

ControllerServiceWorkerConnector::ControllerServiceWorkerConnector(
    mojom::ServiceWorkerContainerHost* container_host,
    mojom::ControllerServiceWorkerPtr controller_ptr)
    : container_host_(container_host) {
  if (controller_ptr) {
    controller_service_worker_ = std::move(controller_ptr);
    controller_service_worker_.set_connection_error_handler(base::BindOnce(
        &ControllerServiceWorkerConnector::OnControllerConnectionClosed,
        base::Unretained(this)));
  }
}

mojom::ControllerServiceWorker*
ControllerServiceWorkerConnector::GetControllerServiceWorker() {
  if (!controller_service_worker_ && container_host_) {
    container_host_->GetControllerServiceWorker(
        mojo::MakeRequest(&controller_service_worker_));
    controller_service_worker_.set_connection_error_handler(base::BindOnce(
        &ControllerServiceWorkerConnector::OnControllerConnectionClosed,
        base::Unretained(this)));
  }
  return controller_service_worker_.get();
}

void ControllerServiceWorkerConnector::AddObserver(Observer* observer) {
  observer_list_.AddObserver(observer);
}

void ControllerServiceWorkerConnector::RemoveObserver(Observer* observer) {
  observer_list_.RemoveObserver(observer);
}

void ControllerServiceWorkerConnector::OnContainerHostConnectionClosed() {
  container_host_ = nullptr;
}

void ControllerServiceWorkerConnector::OnControllerConnectionClosed() {
  controller_service_worker_.reset();
  for (auto& observer : observer_list_)
    observer.OnConnectionClosed();
}

ControllerServiceWorkerConnector::~ControllerServiceWorkerConnector() = default;

}  // namespace content
