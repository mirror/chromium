// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/renderer/blob_storage/blob_url_loader_factory.h"

namespace content {

using blink::mojom::BlobPtr;

RendererBlobURLLoaderFactory::RendererBlobURLLoaderFactory(BlobPtr blob)
    : blob_(std::move(blob)) {}

void RendererBlobURLLoaderFactory::CreateLoaderAndStart(
    network::mojom::URLLoaderRequest loader,
    int32_t routing_id,
    int32_t request_id,
    uint32_t options,
    const network::ResourceRequest& request,
    network::mojom::URLLoaderClientPtr client,
    const net::MutableNetworkTrafficAnnotationTag& traffic_annotation,
    const Constraints& constraints) {
  // TODO(mek): Checks
  DCHECK(request.url.SchemeIsBlob());

  if (request.method != "GET") {
    network::ResourceResponseHead response;
    std::string http_status("HTTP/1.1 405 Method Not Allowed");
    http_status.append("\0\0", 2);
    response.headers =
        base::MakeRefCounted<net::HttpResponseHeaders>(http_status);
    client->OnReceiveResponse(response, base::nullopt, nullptr);

    network::URLLoaderCompletionStatus status;
    // TODO(kinuko): We should probably set the error_code here,
    // while it makes existing tests fail. https://crbug.com/732750
    status.completion_time = base::TimeTicks::Now();
    client->OnComplete(status);
    return;
  }

  blob_->CreateLoader(std::move(loader), request.headers, std::move(client));
}

std::unique_ptr<SharedURLLoaderFactoryInfo>
RendererBlobURLLoaderFactory::Clone() {
  NOTREACHED();
  return nullptr;
}

RendererBlobURLLoaderFactory::~RendererBlobURLLoaderFactory() = default;

}  // namespace content
