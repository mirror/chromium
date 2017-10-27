// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/public/common/simple_url_loader_factory.h"

namespace content {

SimpleURLLoaderFactory::SimpleURLLoaderFactory() = default;

SimpleURLLoaderFactory::~SimpleURLLoaderFactory() = default;

void SimpleURLLoaderFactory::BindRequest(
    mojom::URLLoaderFactoryRequest request) {
  bindings_.AddBinding(this, std::move(request));
}

void SimpleURLLoaderFactory::CreateLoaderAndStart(
    mojom::URLLoaderRequest loader,
    int32_t routing_id,
    int32_t request_id,
    uint32_t options,
    const ResourceRequest& request,
    mojom::URLLoaderClientPtr client,
    const net::MutableNetworkTrafficAnnotationTag& traffic_annotation) {
  CreateLoaderAndStart(request, std::move(loader), std::move(client));
}

void SimpleURLLoaderFactory::Clone(mojom::URLLoaderFactoryRequest request) {
  BindRequest(std::move(request));
}

}  // namespace content
