// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/child/service_worker/controller_service_worker_impl.h"

ControllerServiceWorkerImpl::ControllerServiceWorkerImpl(
    mojom::ControllerServiceWorkerRequest request)
    : weak_factory_(this) {
  bindings_.AddBinding(this, std::move(request));
}

ControllerServiceWorkerImpl::~ControllerServiceWorkerImpl() = default;

void ControllerServiceWorkerImpl::Clone(
    mojom::ControllerServiceWorkerRequest request) {
  bindings_.AddBinding(this, std::move(request));
}

void ControllerServiceWorkerImpl::AddBinding(
    mojom::ControllerServiceWorkerRequest request) {
  bindings_.AddBinding(this, std::move(request));
}

void ControllerServiceWorkerImpl::DispatchFetchEvent(
    const ServiceWorkerFetchRequest& request,
    mojom::ServiceWorkerFetchResponseCallbackPtr response_callback,
    DispatchFetchEventCallback callback) {}
