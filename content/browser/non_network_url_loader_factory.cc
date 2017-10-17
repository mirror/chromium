// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/non_network_url_loader_factory.h"

#include <string>

#include "base/bind.h"
#include "base/macros.h"
#include "content/public/browser/browser_thread.h"
#include "url/gurl.h"

namespace content {

NonNetworkURLLoaderFactory::NonNetworkURLLoaderFactory(
    NonNetworkProtocolHandlerMap protocol_handlers)
    : protocol_handlers_(std::move(protocol_handlers)) {}

void NonNetworkURLLoaderFactory::BindRequest(
    mojom::URLLoaderFactoryRequest request) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  BrowserThread::PostTask(BrowserThread::IO, FROM_HERE,
                          base::BindOnce(&NonNetworkURLLoaderFactory::BindOnIO,
                                         this, std::move(request)));
}

void NonNetworkURLLoaderFactory::ShutDown() {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  BrowserThread::PostTask(
      BrowserThread::IO, FROM_HERE,
      base::BindOnce(&NonNetworkURLLoaderFactory::ShutDownOnIO, this));
}

NonNetworkURLLoaderFactory::~NonNetworkURLLoaderFactory() {}

void NonNetworkURLLoaderFactory::CreateLoaderAndStart(
    mojom::URLLoaderRequest loader,
    int32_t routing_id,
    int32_t request_id,
    uint32_t options,
    const ResourceRequest& request,
    mojom::URLLoaderClientPtr client,
    const net::MutableNetworkTrafficAnnotationTag& traffic_annotation) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);
  std::string scheme = request.url.scheme();
  auto it = protocol_handlers_.find(scheme);
  if (it == protocol_handlers_.end()) {
    DLOG(ERROR) << "No registered handler for '" << scheme << "' scheme of "
                << "request for " << request.url.spec();
    return;
  }

  it->second->CreateAndStartLoader(request, std::move(loader),
                                   std::move(client));
}

void NonNetworkURLLoaderFactory::Clone(mojom::URLLoaderFactoryRequest request) {
  BindRequest(std::move(request));
}

void NonNetworkURLLoaderFactory::BindOnIO(
    mojom::URLLoaderFactoryRequest request) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);
  if (shutting_down_)
    return;
  bindings_.AddBinding(this, std::move(request));
}

void NonNetworkURLLoaderFactory::ShutDownOnIO() {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);
  bindings_.CloseAllBindings();
  protocol_handlers_.clear();
  shutting_down_ = true;
}

}  // namespace content
