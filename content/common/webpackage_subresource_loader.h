// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_COMMON_WEBPACKAGE_SUBRESOURCE_LOADER_H_
#define CONTENT_COMMON_WEBPACKAGE_SUBRESOURCE_LOADER_H_

#include "base/callback.h"
#include "base/memory/ref_counted.h"
#include "base/memory/weak_ptr.h"
#include "content/common/webpackage_response_handler.h"
#include "content/common/webpackage_subresource_manager.mojom.h"
#include "content/public/common/url_loader_factory.mojom.h"
#include "mojo/public/cpp/bindings/binding_set.h"

namespace content {

// This class is self-destruct, when all the bindings are gone this deletes
// itself.
class WebPackageSubresourceLoaderFactory : public mojom::URLLoaderFactory {
 public:
  using ResourceCallback = WebPackageResourceCallback;
  using CompletionCallback = WebPackageCompletionCallback;

  using LoaderFactoryGetter =
      base::RepeatingCallback<mojom::URLLoaderFactory*()>;
  using ResourceGetter =
      base::RepeatingCallback<void(const ResourceRequest& resource_request,
                                   ResourceCallback resource_callback,
                                   CompletionCallback completion_callback)>;

  WebPackageSubresourceLoaderFactory(
      const ResourceGetter& resource_getter,
      const LoaderFactoryGetter& loader_factory_getter);
  ~WebPackageSubresourceLoaderFactory() override;

  // mojom::URLLoaderFactory implementation.
  void CreateLoaderAndStart(mojom::URLLoaderRequest loader_request,
                            int32_t routing_id,
                            int32_t request_id,
                            uint32_t options,
                            const ResourceRequest& resource_request,
                            mojom::URLLoaderClientPtr loader_client,
                            const net::MutableNetworkTrafficAnnotationTag&
                                traffic_annotation) override;
  void Clone(mojom::URLLoaderFactoryRequest request) override;
  void set_allow_fallback(bool flag) { allow_fallback_ = flag; }

 private:
  void OnConnectionError() {
    if (bindings_.empty())
      delete this;
  }

  ResourceGetter resource_getter_;
  LoaderFactoryGetter loader_factory_getter_;

  mojo::BindingSet<mojom::URLLoaderFactory> bindings_;
  bool allow_fallback_ = true;

  base::WeakPtrFactory<WebPackageSubresourceLoaderFactory> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(WebPackageSubresourceLoaderFactory);
};

}  // namespace content

#endif  // CONTENT_COMMON_WEBPACKAGE_SUBRESOURCE_LOADER_H_
