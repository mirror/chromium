// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_NETWORK_NETWORK_SERVICE_URL_LOADER_FACTORY_H_
#define CONTENT_NETWORK_NETWORK_SERVICE_URL_LOADER_FACTORY_H_

#include "base/macros.h"
#include "mojo/public/cpp/bindings/binding_set.h"
#include "net/traffic_annotation/network_traffic_annotation.h"
#include "services/network/public/interfaces/url_loader_factory.mojom.h"

namespace content {

class NetworkContext;

// This class is an implementation of network::mojom::URLLoaderFactory that
// creates a network::mojom::URLLoader. It is responsible for destroying itself
// when all its bindings are closed.
class NetworkServiceURLLoaderFactory : public network::mojom::URLLoaderFactory {
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

 private:
  // Called whenever there's a connection error in one of the elements of
  // |binding_set_|.
  void OnConnectionError();

  // Not owned.
  NetworkContext* context_;
  const uint32_t process_id_;

  mojo::BindingSet<URLLoaderFactory> binding_set_;

  DISALLOW_COPY_AND_ASSIGN(NetworkServiceURLLoaderFactory);
};

}  // namespace content

#endif  // CONTENT_NETWORK_NETWORK_SERVICE_URL_LOADER_FACTORY_H_
