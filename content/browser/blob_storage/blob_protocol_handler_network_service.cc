// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/blob_storage/blob_protocol_handler_network_service.h"

#include <memory>

#include "content/browser/blob_storage/blob_url_loader.h"
#include "content/public/browser/browser_thread.h"
#include "storage/browser/blob/blob_data_handle.h"
#include "storage/browser/blob/blob_storage_context.h"
#include "storage/browser/fileapi/file_system_context.h"

namespace content {

BlobProtocolHandlerNetworkService::BlobProtocolHandlerNetworkService(
    BlobStorageContextGetter blob_context_getter,
    scoped_refptr<storage::FileSystemContext> filesystem_context)
    : file_system_context_(std::move(filesystem_context)), weak_factory_(this) {
  BrowserThread::PostTask(
      BrowserThread::IO, FROM_HERE,
      base::BindOnce(&BlobProtocolHandlerNetworkService::InitializeOnIO,
                     weak_factory_.GetWeakPtr(),
                     std::move(blob_context_getter)));
}

BlobProtocolHandlerNetworkService::~BlobProtocolHandlerNetworkService() =
    default;

void BlobProtocolHandlerNetworkService::CreateAndStartLoader(
    const ResourceRequest& request,
    mojom::URLLoaderRequest loader,
    mojom::URLLoaderClientPtr client) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);
  std::unique_ptr<storage::BlobDataHandle> blob_handle;
  if (blob_storage_context_) {
    blob_handle = blob_storage_context_->GetBlobDataFromPublicURL(request.url);
  }
  BlobURLLoader::CreateAndStart(std::move(loader), request, std::move(client),
                                std::move(blob_handle),
                                file_system_context_.get());
}

void BlobProtocolHandlerNetworkService::InitializeOnIO(
    BlobStorageContextGetter blob_storage_context_getter) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);
  blob_storage_context_ = std::move(blob_storage_context_getter).Run();
}

}  // namespace content
