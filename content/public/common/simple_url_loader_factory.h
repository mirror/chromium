// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_PUBLIC_COMMON_SIMPLE_URL_LOADER_FACTORY_H_
#define CONTENT_PUBLIC_COMMON_SIMPLE_URL_LOADER_FACTORY_H_

#include "base/macros.h"
#include "content/common/content_export.h"
#include "content/public/common/url_loader_factory.mojom.h"
#include "mojo/public/cpp/bindings/binding_set.h"

namespace content {

// A wrapper around mojom::URLLoaderFactory which simplifies the burden of
// implementation for the simpler use cases. Manages a BindingSet and elides
// certain interface parameters which most factories can ignore.
class CONTENT_EXPORT SimpleURLLoaderFactory : public mojom::URLLoaderFactory {
 public:
  SimpleURLLoaderFactory();
  ~SimpleURLLoaderFactory() override;

  // Must be implemented by the subclass to handle new incoming URLLoader
  // requests.
  virtual void CreateLoaderAndStart(const ResourceRequest& request,
                                    mojom::URLLoaderRequest loader,
                                    mojom::URLLoaderClientPtr client) = 0;

  // Binds a new URLLoaderFactoryRequest to this factory.
  void BindRequest(mojom::URLLoaderFactoryRequest request);

 private:
  // mojom::URLLoaderFactory:
  void CreateLoaderAndStart(mojom::URLLoaderRequest loader,
                            int32_t routing_id,
                            int32_t request_id,
                            uint32_t options,
                            const ResourceRequest& request,
                            mojom::URLLoaderClientPtr client,
                            const net::MutableNetworkTrafficAnnotationTag&
                                traffic_annotation) override;
  void Clone(mojom::URLLoaderFactoryRequest request) override;

  mojo::BindingSet<mojom::URLLoaderFactory> bindings_;

  DISALLOW_COPY_AND_ASSIGN(SimpleURLLoaderFactory);
};

}  // namespace content

#endif  // CONTENT_PUBLIC_COMMON_SIMPLE_URL_LOADER_FACTORY_H_
