// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_LOADER_SIGNED_EXCHANGE_PREFETCH_SERVICE_H_
#define CONTENT_BROWSER_LOADER_SIGNED_EXCHANGE_PREFETCH_SERVICE_H_

#include <string>
#include <vector>

#include "mojo/public/cpp/bindings/strong_binding.h"
#include "services/network/public/interfaces/url_loader_factory.mojom.h"
#include "third_party/WebKit/public/platform/modules/htxg/signed_exchange_prefetch_service.mojom.h"

namespace network {
namespace mojom {

class NetworkContext;

}  // namespace mojom
}  // namespace network

namespace content {

class StoragePartition;

class SignedExchangePrefetchServiceImpl
    : public blink::mojom::SignedExchangePrefetchService {
 public:
  explicit SignedExchangePrefetchServiceImpl(
      network::mojom::NetworkContext* network_context,
      int process_id);
  ~SignedExchangePrefetchServiceImpl() override;

  static void CreateMojoService(
      StoragePartition* storage_partition,
      int process_id,
      blink::mojom::SignedExchangePrefetchServiceRequest request);

  void StartPrefetch(
      blink::mojom::SignedExchangePrefetchLoaderRequest prefetch_loader_request,
      blink::mojom::SignedExchangePrefetchLoaderClientPtr
          prefetch_loader_client,
      const GURL& url) override;

 private:
  network::mojom::URLLoaderFactoryPtr url_loader_factory_;
};

}  // namespace content

#endif  // CONTENT_BROWSER_LOADER_SIGNED_EXCHANGE_PREFETCH_SERVICE_H_
