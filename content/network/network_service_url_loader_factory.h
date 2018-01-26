// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_NETWORK_NETWORK_SERVICE_URL_LOADER_FACTORY_H_
#define CONTENT_NETWORK_NETWORK_SERVICE_URL_LOADER_FACTORY_H_

#include "base/macros.h"
#include "content/common/content_export.h"
#include "mojo/public/cpp/bindings/binding_set.h"
#include "net/traffic_annotation/network_traffic_annotation.h"
#include "services/network/public/interfaces/url_loader_factory.mojom.h"

namespace content {

class NetworkContext;
class URLLoader;

// This class is an implementation of network::mojom::URLLoaderFactory that
// creates a network::mojom::URLLoader. It is responsible for destroying itself
// when all its bindings are closed.
class CONTENT_EXPORT NetworkServiceURLLoaderFactory
    : public network::mojom::URLLoaderFactory {
 public:
  // NOTE: |context| must outlive this instance.
  NetworkServiceURLLoaderFactory(
      NetworkContext* context,
      uint32_t process_id,
      network::mojom::URLLoaderFactoryRequest request);

  ~NetworkServiceURLLoaderFactory() override;

  // Called when the associated NetworkContext is going away.
  void Cleanup();

  // network::mojom::URLLoaderFactory implementation.
  void CreateLoaderAndStart(network::mojom::URLLoaderRequest request,
                            int32_t routing_id,
                            int32_t request_id,
                            uint32_t options,
                            const network::ResourceRequest& url_request,
                            network::mojom::URLLoaderClientPtr client,
                            const net::MutableNetworkTrafficAnnotationTag&
                                traffic_annotation) override;
  void Clone(network::mojom::URLLoaderFactoryRequest request) override;

  // These are called by individual url loaders as they are being created and
  // destroyed.
  void RegisterURLLoader(URLLoader* url_loader);
  void DeregisterURLLoader(URLLoader* url_loader);

 private:
  // Destroys |this| if there are no active URLLoaders and no live
  // URLLoaderFactory bindings.
  void DestroyIfNotInUse();

  // Not owned.
  NetworkContext* context_;
  const uint32_t process_id_;

  // URLLoaders register themselves with the URLLoaderFactory so that they can
  // keep the factory alive and can be cleaned up when the NetworkContext goes
  // away. This is needed as net::URLRequests held by URLLoaders have to be gone
  // when net::URLRequestContext (held by NetworkContext) is destroyed.
  std::set<URLLoader*> url_loaders_;

  mojo::BindingSet<URLLoaderFactory> binding_set_;

  DISALLOW_COPY_AND_ASSIGN(NetworkServiceURLLoaderFactory);
};

}  // namespace content

#endif  // CONTENT_NETWORK_NETWORK_SERVICE_URL_LOADER_FACTORY_H_
