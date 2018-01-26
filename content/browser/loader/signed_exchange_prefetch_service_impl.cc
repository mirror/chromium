// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/loader/signed_exchange_prefetch_service_impl.h"

#include "content/public/browser/storage_partition.h"
#include "services/network/public/interfaces/network_service.mojom.h"

namespace content {

SignedExchangePrefetchServiceImpl::SignedExchangePrefetchServiceImpl(
    network::mojom::NetworkContext* network_context,
    int process_id) {
  network_context->CreateURLLoaderFactory(
      mojo::MakeRequest(&url_loader_factory_), process_id);
}

SignedExchangePrefetchServiceImpl::~SignedExchangePrefetchServiceImpl() =
    default;

// static
void SignedExchangePrefetchServiceImpl::CreateMojoService(
    StoragePartition* storage_partition,
    int process_id,
    blink::mojom::SignedExchangePrefetchServiceRequest request) {
  LOG(ERROR) << "SignedExchangePrefetchServiceImpl::CreateMojoService";
  mojo::MakeStrongBinding(
      std::make_unique<SignedExchangePrefetchServiceImpl>(
          storage_partition->GetNetworkContext(), process_id),
      std::move(request));
}

void SignedExchangePrefetchServiceImpl::Prefetch(
    const GURL& url,
    blink::mojom::SignedExchangePrefetchAborterRequest aborter) {
  LOG(ERROR) << "SignedExchangePrefetchServiceImpl::Prefetch " << url;
}

}  // namespace content
