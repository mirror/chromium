// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_CHILD_URL_LOADER_FACTORY_ADAPTER_H_
#define CONTENT_CHILD_URL_LOADER_FACTORY_ADAPTER_H_

#include "content/public/common/url_loader.mojom.h"

#include "content/common/possibly_associated_interface_ptr.h"

namespace content {

// A helper class for creating and starting a URLLoader, for allowing
// both associated mojo URLLoaderFactory and custom C++ URLLoader factory
// class to use the same code.
class URLLoaderFactoryAdapter {
 public:
  using PossiblyAssociatedURLLoaderFactoryPtr =
      PossiblyAssociatedInterfacePtr<mojom::URLLoaderFactory>;

  explicit URLLoaderFactoryAdapter(
      PossiblyAssociatedInterfacePtr loader_factory)
      : loader_factory_(std::move(loader_factory)) {}

  void CreateLoaderAndStart(int routing_id,
                            int request_id,
                            uint32_t options,
                            const ResourceRequest& request,
                            mojom::URLLoaderRequest request,
                            mojom::URLLoaderClientPtr client) {}

 private:
  PossiblyAssociatedInterfacePtr<mojom::URLLoaderFactory> loader_factory_;
};

}  // namespace content

#endif  // CONTENT_CHILD_URL_LOADER_FACTORY_ADAPTER_H_
