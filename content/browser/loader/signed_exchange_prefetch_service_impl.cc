// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/loader/signed_exchange_prefetch_service_impl.h"

#include "content/public/browser/storage_partition.h"
#include "mojo/public/cpp/bindings/strong_binding.h"
#include "services/network/public/interfaces/network_service.mojom.h"

namespace content {

namespace {

class SignedExchangePrefetchLoaderImpl
    : public blink::mojom::SignedExchangePrefetchLoader {
 public:
  SignedExchangePrefetchLoaderImpl(
      blink::mojom::SignedExchangePrefetchLoaderClientPtr
          prefetch_loader_client,
      const GURL& url)
      : prefetch_loader_client_(std::move(prefetch_loader_client)) {
    LOG(ERROR)
        << "SignedExchangePrefetchLoaderImpl::SignedExchangePrefetchLoaderImpl "
        << this << " " << url;
  }
  ~SignedExchangePrefetchLoaderImpl() override {
    LOG(ERROR) << "SignedExchangePrefetchLoaderImpl::~"
                  "SignedExchangePrefetchLoaderImpl "
               << this;
  }
  void Abort() override {
    LOG(ERROR) << "SignedExchangePrefetchLoaderImpl::Abort " << this;
  }
  blink::mojom::SignedExchangePrefetchLoaderClientPtr prefetch_loader_client_;
};

}  // namespace

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

void SignedExchangePrefetchServiceImpl::StartPrefetch(
    blink::mojom::SignedExchangePrefetchLoaderRequest prefetch_loader_request,
    blink::mojom::SignedExchangePrefetchLoaderClientPtr prefetch_loader_client,
    const GURL& url) {
  LOG(ERROR) << "SignedExchangePrefetchServiceImpl::StartPrefetch " << url;

  std::unique_ptr<SignedExchangePrefetchLoaderImpl> loader =
      base::MakeUnique<SignedExchangePrefetchLoaderImpl>(
          std::move(prefetch_loader_client), url);

  mojo::MakeStrongBinding(std::move(loader),
                          std::move(prefetch_loader_request));
}

}  // namespace content
