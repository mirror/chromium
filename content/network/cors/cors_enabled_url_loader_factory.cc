// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/network/cors/cors_enabled_url_loader_factory.h"
#include "content/network/cors/cors_url_loader.h"
#include "content/public/common/content_features.h"

namespace content {

class CORSEnabledURLLoaderFactory::NetworkLoaderFactoryForCORSURLLoader final
    : public mojom::URLLoaderFactory {
 public:
  NetworkLoaderFactoryForCORSURLLoader(CORSEnabledURLLoaderFactory* impl)
      : impl_(impl) {
    DCHECK(impl);
  };
  ~NetworkLoaderFactoryForCORSURLLoader() override = default;

  void CreateLoaderAndStart(mojom::URLLoaderRequest request,
                            int32_t routing_id,
                            int32_t request_id,
                            uint32_t options,
                            const network::ResourceRequest& resource_request,
                            mojom::URLLoaderClientPtr client,
                            const net::MutableNetworkTrafficAnnotationTag&
                                traffic_annotation) override {
    impl_->CreateNoCORSLoaderAndStart(std::move(request), routing_id,
                                      request_id, options, resource_request,
                                      std::move(client), traffic_annotation);
  }

 private:
  void Clone(mojom::URLLoaderFactoryRequest factory) override { NOTREACHED(); }

  CORSEnabledURLLoaderFactory* impl_;
};

CORSEnabledURLLoaderFactory::CORSEnabledURLLoaderFactory()
    : network_loader_factory_(
          std::make_unique<NetworkLoaderFactoryForCORSURLLoader>(this)) {}

CORSEnabledURLLoaderFactory::~CORSEnabledURLLoaderFactory() = default;

void CORSEnabledURLLoaderFactory::CreateLoaderAndStart(
    mojom::URLLoaderRequest request,
    int32_t routing_id,
    int32_t request_id,
    uint32_t options,
    const network::ResourceRequest& resource_request,
    mojom::URLLoaderClientPtr client,
    const net::MutableNetworkTrafficAnnotationTag& traffic_annotation) {
  if (base::FeatureList::IsEnabled(features::kOutOfBlinkCORS)) {
    loader_bindings_.AddBinding(
        std::make_unique<CORSURLLoader>(routing_id, request_id, options,
                                        resource_request, std::move(client),
                                        traffic_annotation,
                                        network_loader_factory_.get()),
        std::move(request));
  } else {
    CreateNoCORSLoaderAndStart(std::move(request), routing_id, request_id,
                               options, resource_request, std::move(client),
                               traffic_annotation);
  }
}

}  // namespace content
