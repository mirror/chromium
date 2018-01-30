// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/public/common/shared_url_loader_factory.h"
#include "third_party/WebKit/common/blob/blob.mojom.h"

#ifndef CONTENT_RENDERER_BLOB_STORAGE_BLOB_URL_LOADER_FACTORY_H_
#define CONTENT_RENDERER_BLOB_STORAGE_BLOB_URL_LOADER_FACTORY_H_

namespace content {

class RendererBlobURLLoaderFactory : public SharedURLLoaderFactory {
 public:
  explicit RendererBlobURLLoaderFactory(blink::mojom::BlobPtr blob);

  // SharedURLLoaderFactory:
  void CreateLoaderAndStart(
      network::mojom::URLLoaderRequest loader,
      int32_t routing_id,
      int32_t request_id,
      uint32_t options,
      const network::ResourceRequest& request,
      network::mojom::URLLoaderClientPtr client,
      const net::MutableNetworkTrafficAnnotationTag& traffic_annotation,
      const Constraints& constraints) override;
  std::unique_ptr<SharedURLLoaderFactoryInfo> Clone() override;

 protected:
  ~RendererBlobURLLoaderFactory() override;

 private:
  blink::mojom::BlobPtr blob_;
};

}  // namespace content

#endif  // CONTENT_RENDERER_BLOB_STORAGE_BLOB_URL_LOADER_FACTORY_H_
