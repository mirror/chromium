// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICES_NETWORK_NETWORK_SERVICE_URL_LOADER_FACTORY_H_
#define SERVICES_NETWORK_NETWORK_SERVICE_URL_LOADER_FACTORY_H_

#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "mojo/public/cpp/bindings/binding_set.h"
#include "net/traffic_annotation/network_traffic_annotation.h"
#include "services/network/public/interfaces/url_loader_factory.mojom.h"

namespace network {

class NetworkContext;
class URLLoader;

// This class is an implementation of mojom::URLLoaderFactory that
// creates a mojom::URLLoader. It is responsible for destroying itself when all
// its bindings are closed.
class NetworkServiceURLLoaderFactory : public network::mojom::URLLoaderFactory {
 public:
  // NOTE: |context| must outlive this instance.
  NetworkServiceURLLoaderFactory(NetworkContext* context,
                                 uint32_t process_id,
                                 mojom::URLLoaderFactoryRequest request);

  ~NetworkServiceURLLoaderFactory() override;

  // Called when the associated NetworkContext is going away.
  void Cleanup();

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

  // These are called by individual url loaders as they are being created and
  // destroyed.
  void RegisterURLLoader(URLLoader* url_loader);
  void DeregisterURLLoader(URLLoader* url_loader);

 private:
  // Posts task to destroys |this| if there are no active URLLoaders and no live
  // URLLoaderFactory bindings. Done asynchronously to avoid calling it when
  // tearing down URLLoaders in the destructor.
  void PostDestroyTaskIfNotInUse();

  // Deletes |this|. Should only be posted as a task by
  // PostDestroyTaskIfNotInUse().
  void DestroySelf();

  // Not owned.
  NetworkContext* context_;
  const uint32_t process_id_;

  // URLLoaders register themselves with the URLLoaderFactory so that they can
  // keep the factory alive and can be cleaned up when the NetworkContext goes
  // away. This is needed as net::URLRequests held by URLLoaders have to be gone
  // when net::URLRequestContext (held by NetworkContext) is destroyed.
  std::set<URLLoader*> url_loaders_;

  mojo::BindingSet<URLLoaderFactory> binding_set_;

  base::WeakPtrFactory<NetworkServiceURLLoaderFactory> weak_ptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(NetworkServiceURLLoaderFactory);
};

}  // namespace network

#endif  // SERVICES_NETWORK_NETWORK_SERVICE_URL_LOADER_FACTORY_H_
