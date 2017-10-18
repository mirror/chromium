// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/child/service_worker/controller_service_worker_connector.h"

#include "base/bind.h"
#include "base/bind_helpers.h"
#include "content/common/service_worker/service_worker_container.mojom.h"
#include "mojo/public/cpp/bindings/interface_request.h"

namespace content {

ControllerServiceWorkerConnector::ControllerServiceWorkerConnector(
    mojom::ServiceWorkerContainerHost* container_host)
    : container_host_(container_host) {}

mojom::ControllerServiceWorker*
ControllerServiceWorkerConnector::GetControllerServiceWorker() {
  if (!controller_service_worker_ && container_host_) {
    // TODO(kinuko): For simplicity we always call GetControllerServiceWorker
    // for now, but it should be possible to give the initial
    // controller_service_worker_ in ctor (by sending it from the browser
    // process when a new controller is set).
    container_host_->GetControllerServiceWorker(
        mojo::MakeRequest(&controller_service_worker_));
    controller_service_worker_.set_connection_error_handler(base::BindOnce(
        &ControllerServiceWorkerConnector::OnControllerConnectionClosed,
        base::Unretained(this)));
  }
  return controller_service_worker_.get();
}

void ControllerServiceWorkerConnector::AddControllerConnectionErrorHandler(
    uint64_t id,
    base::RepeatingClosure handler) {
  DCHECK(!base::ContainsKey(controller_connection_error_handlers_, id));
  controller_connection_error_handlers_[id] = handler;
}

void ControllerServiceWorkerConnector::RemoveControllerConnectionErrorHandler(
    uint64_t id) {
  DCHECK(base::ContainsKey(controller_connection_error_handlers_, id));
  controller_connection_error_handlers_.erase(id);
}

void ControllerServiceWorkerConnector::OnContainerHostConnectionClosed() {
  container_host_ = nullptr;
}

ControllerServiceWorkerConnector::~ControllerServiceWorkerConnector() = default;

void ControllerServiceWorkerConnector::OnControllerConnectionClosed() {
  controller_service_worker_.reset();
  for (const auto& handler : controller_connection_error_handlers_)
    handler.second.Run();
}

}  // namespace content
