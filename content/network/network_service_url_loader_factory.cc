// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/network/network_service_url_loader_factory.h"

#include "base/logging.h"
#include "content/network/network_context.h"
#include "content/network/network_service_impl.h"
#include "content/network/url_loader.h"
#include "services/network/public/cpp/resource_request.h"

namespace content {

NetworkServiceURLLoaderFactory::NetworkServiceURLLoaderFactory(
    NetworkContext* context,
    uint32_t process_id,
    network::mojom::URLLoaderFactoryRequest request)
    : context_(context), process_id_(process_id) {
  binding_set_.set_connection_error_handler(
      base::BindRepeating(&NetworkServiceURLLoaderFactory::DestroyIfNotInUse,
                          base::Unretained(this)));
  binding_set_.AddBinding(this, std::move(request));
  context_->RegisterURLLoaderFactory(this);
}

NetworkServiceURLLoaderFactory::~NetworkServiceURLLoaderFactory() {
  // Call each URLLoader and ask it to release its net::URLRequest, as the
  // corresponding net::URLRequestContext is going away with this
  // NetworkContext. The loaders can be deregistering themselves in Cleanup(),
  // so have to be careful.
  while (!url_loaders_.empty())
    (*url_loaders_.begin())->Cleanup();

  context_->DeregisterURLLoaderFactory(this);
}

void NetworkServiceURLLoaderFactory::Cleanup() {
  delete this;
}

void NetworkServiceURLLoaderFactory::CreateLoaderAndStart(
    network::mojom::URLLoaderRequest request,
    int32_t routing_id,
    int32_t request_id,
    uint32_t options,
    const network::ResourceRequest& url_request,
    network::mojom::URLLoaderClientPtr client,
    const net::MutableNetworkTrafficAnnotationTag& traffic_annotation) {
  bool report_raw_headers = false;
  if (url_request.report_raw_headers) {
    const NetworkServiceImpl* service = context_->network_service();
    report_raw_headers = service && service->HasRawHeadersAccess(process_id_);
    if (!report_raw_headers)
      DLOG(ERROR) << "Denying raw headers request by process " << process_id_;
  }
  new URLLoader(
      this, context_, std::move(request), options, url_request,
      report_raw_headers, std::move(client),
      static_cast<net::NetworkTrafficAnnotationTag>(traffic_annotation),
      process_id_);
}

void NetworkServiceURLLoaderFactory::Clone(
    network::mojom::URLLoaderFactoryRequest request) {
  binding_set_.AddBinding(this, std::move(request));
}

void NetworkServiceURLLoaderFactory::RegisterURLLoader(URLLoader* url_loader) {
  DCHECK(url_loaders_.count(url_loader) == 0);
  url_loaders_.insert(url_loader);
}

void NetworkServiceURLLoaderFactory::DeregisterURLLoader(
    URLLoader* url_loader) {
  size_t removed_count = url_loaders_.erase(url_loader);
  DCHECK(removed_count);

  DestroyIfNotInUse();
}

void NetworkServiceURLLoaderFactory::DestroyIfNotInUse() {
  if (binding_set_.empty() && binding_set_.empty())
    delete this;
}

}  // namespace content
