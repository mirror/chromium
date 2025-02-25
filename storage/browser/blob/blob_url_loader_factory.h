// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef STORAGE_BROWSER_BLOB_BLOB_URL_LOADER_FACTORY_H_
#define STORAGE_BROWSER_BLOB_BLOB_URL_LOADER_FACTORY_H_

#include "mojo/public/cpp/bindings/binding_set.h"
#include "services/network/public/interfaces/url_loader_factory.mojom.h"
#include "storage/browser/storage_browser_export.h"

namespace storage {

class BlobDataHandle;

// URLLoaderFactory that can create loaders for exactly one url, loading the
// blob that was passed to its constructor. This factory keeps the blob alive.
// Self destroys when no more bindings exist.
class STORAGE_EXPORT BlobURLLoaderFactory
    : public network::mojom::URLLoaderFactory {
 public:
  static void Create(std::unique_ptr<BlobDataHandle> handle,
                     const GURL& blob_url,
                     network::mojom::URLLoaderFactoryRequest request);

  // URLLoaderFactory:
  void CreateLoaderAndStart(network::mojom::URLLoaderRequest loader,
                            int32_t routing_id,
                            int32_t request_id,
                            uint32_t options,
                            const network::ResourceRequest& request,
                            network::mojom::URLLoaderClientPtr client,
                            const net::MutableNetworkTrafficAnnotationTag&
                                traffic_annotation) override;
  void Clone(network::mojom::URLLoaderFactoryRequest request) override;

 private:
  BlobURLLoaderFactory(std::unique_ptr<BlobDataHandle> handle,
                       const GURL& blob_url,
                       network::mojom::URLLoaderFactoryRequest request);
  ~BlobURLLoaderFactory() override;
  void OnConnectionError();

  std::unique_ptr<BlobDataHandle> handle_;
  GURL url_;

  mojo::BindingSet<network::mojom::URLLoaderFactory> bindings_;

  DISALLOW_COPY_AND_ASSIGN(BlobURLLoaderFactory);
};

}  // namespace storage

#endif  // STORAGE_BROWSER_BLOB_BLOB_URL_LOADER_FACTORY_H_
