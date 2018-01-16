// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_NETWORK_CORS_CORS_ENABLED_URL_LOADER_FACTORY_H_
#define CONTENT_NETWORK_CORS_CORS_ENABLED_URL_LOADER_FACTORY_H_

#include <memory>

#include "base/macros.h"
#include "content/common/content_export.h"
#include "content/public/common/url_loader_factory.mojom.h"
#include "mojo/public/cpp/bindings/strong_binding_set.h"
#include "net/traffic_annotation/network_traffic_annotation.h"

namespace network {
struct ResourceRequest;
}  // namespace network

namespace content {

class CONTENT_EXPORT CORSEnabledURLLoaderFactory
    : public mojom::URLLoaderFactory {
 public:
  void CreateLoaderAndStart(
      mojom::URLLoaderRequest request,
      int32_t routing_id,
      int32_t request_id,
      uint32_t options,
      const network::ResourceRequest& resource_request,
      mojom::URLLoaderClientPtr client,
      const net::MutableNetworkTrafficAnnotationTag& traffic_annotation) final;

 protected:
  CORSEnabledURLLoaderFactory();
  ~CORSEnabledURLLoaderFactory() override;

 private:
  class NetworkLoaderFactoryForCORSURLLoader;
  friend class NetworkLoaderFactoryForCORSURLLoader;

  virtual void CreateNoCORSLoaderAndStart(
      mojom::URLLoaderRequest request,
      int32_t routing_id,
      int32_t request_id,
      uint32_t options,
      const network::ResourceRequest& resource_request,
      mojom::URLLoaderClientPtr client,
      const net::MutableNetworkTrafficAnnotationTag& traffic_annotation) = 0;

  std::unique_ptr<NetworkLoaderFactoryForCORSURLLoader> network_loader_factory_;

  // The factory owns the CORSURLLoader it creates.
  mojo::StrongBindingSet<mojom::URLLoader> loader_bindings_;

  DISALLOW_COPY_AND_ASSIGN(CORSEnabledURLLoaderFactory);
};

}  // namespace content

#endif  // CONTENT_NETWORK_CORS_CORS_ENABLED_URL_LOADER_FACTORY_H_
