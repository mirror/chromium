// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICES_NETWORK_NETWORK_SERVICE_URL_LOADER_FACTORY_H_
#define SERVICES_NETWORK_NETWORK_SERVICE_URL_LOADER_FACTORY_H_

#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "net/traffic_annotation/network_traffic_annotation.h"
#include "services/network/public/interfaces/url_loader_factory.mojom.h"

namespace network {

class NetworkContext;
class ResourceSchedulerClient;

// This class is an implementation of mojom::URLLoaderFactory that
// creates a mojom::URLLoader.
// A NetworkServiceURLLoaderFactory has a pointer to ResourceSchedulerClient. A
// ResourceSchedulerClient is associated with cloned
// NetworkServiceURLLoaderFactories. Roughly one NetworkServiceURLLoaderFactory
// is created for one frame in render process, so it means ResourceScheduler
// works on each frame.
class NetworkServiceURLLoaderFactory : public mojom::URLLoaderFactory {
 public:
  // NOTE: |context| must outlive this instance.
  NetworkServiceURLLoaderFactory(
      NetworkContext* context,
      uint32_t process_id,
      scoped_refptr<ResourceSchedulerClient> resource_scheduler_clien);

  ~NetworkServiceURLLoaderFactory() override;

  // mojom::URLLoaderFactory implementation.
  void CreateLoaderAndStart(mojom::URLLoaderRequest request,
                            int32_t routing_id,
                            int32_t request_id,
                            uint32_t options,
                            const ResourceRequest& url_request,
                            mojom::URLLoaderClientPtr client,
                            const net::MutableNetworkTrafficAnnotationTag&
                                traffic_annotation) override;
  void Clone(mojom::URLLoaderFactoryRequest request) override;

 private:
  // Not owned.
  NetworkContext* context_;
  uint32_t process_id_;
  scoped_refptr<ResourceSchedulerClient> resource_scheduler_client_;

  DISALLOW_COPY_AND_ASSIGN(NetworkServiceURLLoaderFactory);
};

}  // namespace network

#endif  // SERVICES_NETWORK_NETWORK_SERVICE_URL_LOADER_FACTORY_H_
