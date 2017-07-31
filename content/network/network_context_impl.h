// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_NETWORK_NETWORK_CONTEXT_IMPL_H_
#define CONTENT_NETWORK_NETWORK_CONTEXT_IMPL_H_

#include <stdint.h>

#include <memory>
#include <set>

#include "base/macros.h"
#include "content/common/content_export.h"
#include "content/public/common/network_service.mojom.h"
#include "content/public/common/url_loader_factory.mojom.h"
#include "content/public/network/network_context.h"
#include "mojo/public/cpp/bindings/binding.h"
#include "mojo/public/cpp/bindings/strong_binding_set.h"

namespace net {
class URLRequestContext;
class URLRequestContextBuilder;
}  // namespace net

namespace content {
class NetworkServiceImpl;
class URLLoaderImpl;

// A NetworkContextImpl creates and manages access to a URLRequestContext.
//
// When the network service is enabled, NetworkContextImpls are created through
// NetworkService's mojo interface and are owned jointly by the NetworkService
// and the NetworkContextImplPtr used to talk to them, and the
// NetworkContextImpl is destroyed when either one is torn down.
//
// When the network service is disabled, NetworkContextImpls may be created
// through NetworkServiceImpl::CreateNetworkContextImplWithBuilder, and take in
// a URLRequestContextBuilder to seed construction of the NetworkContextImpl's
// URLRequestContext. When that happens, the consumer takes ownership of the
// NetworkContextImpl directly, has direct access to its URLRequestContext, and
// is responsible for destroying it before the NetworkService.
class CONTENT_EXPORT NetworkContextImpl : public NetworkContext {
 public:
  NetworkContextImpl(NetworkServiceImpl* network_service,
                     mojom::NetworkContextRequest request,
                     mojom::NetworkContextParamsPtr params);

  // Temporary constructor that allows creating an in-process NetworkContextImpl
  // with a pre-populated URLRequestContextBuilder.
  NetworkContextImpl(mojom::NetworkContextRequest request,
                     mojom::NetworkContextParamsPtr params,
                     std::unique_ptr<net::URLRequestContextBuilder> builder);

  // Creates a NetworkContext that wraps a consumer-provided URLRequestContext
  // that the NetworkContext does not own. In this case, there is no
  // NetworkService object.
  // TODO(mmenke):  Remove this constructor when the network service ships.
  NetworkContextImpl(mojom::NetworkContextRequest request,
                     net::URLRequestContext* url_request_context);

  ~NetworkContextImpl() override;

  static std::unique_ptr<NetworkContextImpl> CreateForTesting();

  net::URLRequestContext* url_request_context() { return url_request_context_; }

  // These are called by individual url loaders as they are being created and
  // destroyed.
  void RegisterURLLoader(URLLoaderImpl* url_loader);
  void DeregisterURLLoader(URLLoaderImpl* url_loader);

  // mojom::NetworkContext implementation:
  void CreateURLLoaderFactory(mojom::URLLoaderFactoryRequest request,
                              uint32_t process_id) override;
  void HandleViewCacheRequest(const GURL& url,
                              mojom::URLLoaderClientPtr client) override;

  // Called when the associated NetworkServiceImpl is going away. Guaranteed to
  // destroy NetworkContextImpl's URLRequestContext.
  void Cleanup();

 private:
  NetworkContextImpl();

  // On connection errors the NetworkContextImpl destroys itself.
  void OnConnectionError();

  NetworkServiceImpl* const network_service_;

  const mojom::NetworkContextParamsPtr params_;

  // Owning pointer to |url_request_context_|. nullptr when the
  // NetworkContextImpl doesn't own its own URLRequestContext.
  std::unique_ptr<net::URLRequestContext> owned_url_request_context_;

  net::URLRequestContext* url_request_context_ = nullptr;

  // Put it below |url_request_context_| so that it outlives all the
  // NetworkServiceURLLoaderFactoryImpl instances.
  mojo::StrongBindingSet<mojom::URLLoaderFactory> loader_factory_bindings_;

  // URLLoaderImpls register themselves with the NetworkContextImpl so that they
  // can be cleaned up when the NetworkContextImpl goes away. This is needed as
  // net::URLRequests held by URLLoaderImpls have to be gone when
  // net::URLRequestContext (held by NetworkContextImpl) is destroyed.
  std::set<URLLoaderImpl*> url_loaders_;

  mojo::Binding<mojom::NetworkContext> binding_;

  DISALLOW_COPY_AND_ASSIGN(NetworkContextImpl);
};

}  // namespace content

#endif  // CONTENT_NETWORK_NETWORK_CONTEXT_IMPL_H_
