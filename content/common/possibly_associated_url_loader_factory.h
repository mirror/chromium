// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_COMMON_POSSIBLY_ASSOCIATED_URL_LOADER_FACTORY_H_
#define CONTENT_COMMON_POSSIBLY_ASSOCIATED_URL_LOADER_FACTORY_H_

#include "content/public/common/url_loader_factory.mojom.h"

#include "content/common/possibly_associated_interface_ptr.h"

namespace content {

// URLLoaderFactory-class that takes independent URLLoader requests.
// TODO(kinuko): This and the wrappers below should go away once all
// URLLoaderFactory takes independent URLLoader request.
class IndependentURLLoaderFactory {
 public:
  virtual ~IndependentURLLoaderFactory() {}
  virtual void CreateLoaderAndStart(
      mojom::URLLoaderRequest request,
      int32_t routing_id,
      int32_t request_id,
      uint32_t options,
      const ResourceRequest& resource_request,
      mojom::URLLoaderClientPtr client,
      const net::MutableNetworkTrafficAnnotationTag& traffic_annotation) = 0;
  virtual bool SyncLoad(int32_t routing_id,
                        int32_t request_id,
                        const ResourceRequest& request,
                        SyncLoadResult* result) = 0;
};

// A wrapper class that could be useful when we want to handle both associated
// and non-associated URLLoader requests.
class PossiblyAssociatedURLLoaderFactory {
 public:
  PossiblyAssociatedURLLoaderFactory() = default;
  explicit PossiblyAssociatedURLLoaderFactory(mojom::URLLoaderFactory* factory)
      : associated_factory_(factory) {}
  explicit PossiblyAssociatedURLLoaderFactory(
      IndependentURLLoaderFactory* factory)
      : independent_factory_(factory) {}

  PossiblyAssociatedInterfacePtr<mojom::URLLoader> CreateLoaderAndStart(
      int32_t routing_id,
      int32_t request_id,
      uint32_t options,
      const ResourceRequest& resource_request,
      mojom::URLLoaderClientPtr client,
      const net::MutableNetworkTrafficAnnotationTag& traffic_annotation) {
    if (associated_factory_) {
      mojom::URLLoaderAssociatedPtr url_loader;
      associated_factory_->CreateLoaderAndStart(
          mojo::MakeRequest(&url_loader), routing_id, request_id, options,
          resource_request, std::move(client), traffic_annotation);
      return PossiblyAssociatedInterfacePtr<mojom::URLLoader>(
          std::move(url_loader));
    }
    mojom::URLLoaderPtr url_loader;
    independent_factory_->CreateLoaderAndStart(
        mojo::MakeRequest(&url_loader), routing_id, request_id, options,
        resource_request, std::move(client), traffic_annotation);
    return PossiblyAssociatedInterfacePtr<mojom::URLLoader>(
        std::move(url_loader));
  }

  bool SyncLoad(int32_t routing_id,
                int32_t request_id,
                const ResourceRequest& request,
                SyncLoadResult* result) {
    if (associated_factory_)
      return associated_factory_->SyncLoad(routing_id, request_id, request,
                                           result);
    return independent_factory_->SyncLoad(routing_id, request_id, request,
                                          result);
  }

  explicit operator bool() const {
    return associated_factory_ || independent_factory_;
  }

 private:
  mojom::URLLoaderFactory* associated_factory_ = nullptr;
  IndependentURLLoaderFactory* independent_factory_ = nullptr;
};

}  // namespace content

#endif  // CONTENT_COMMON_POSSIBLY_ASSOCIATED_URL_LOADER_FACTORY_H_
