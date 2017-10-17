// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/non_network_url_loader_factory.h"

#include "base/macros.h"
#include "content/browser/blob_storage/blob_url_loader.h"
#include "storage/browser/blob/blob_data_handle.h"
#include "storage/browser/blob/blob_storage_context.h"
#include "storage/browser/fileapi/file_system_context.h"
#include "url/gurl.h"

namespace content {

// static
scoped_refptr<NonNetworkURLLoaderFactory> NonNetworkURLLoaderFactory::Create(
    BlobContextGetter blob_context_getter,
    scoped_refptr<storage::FileSystemContext> file_system_context) {
  scoped_refptr<NonNetworkURLLoaderFactory> factory =
      new NonNetworkURLLoaderFactory(std::move(file_system_context));
  BrowserThread::PostTask(
      BrowserThread::IO, FROM_HERE,
      base::BindOnce(&NonNetworkURLLoaderFactory::InitializeOnIO, factory,
                     std::move(blob_context_getter)));
  return factory;
}

NonNetworkURLLoaderFactory::NonNetworkURLLoaderFactory(
    scoped_refptr<storage::FileSystemContext> file_system_context)
    : file_system_context_(std::move(file_system_context)) {}

NonNetworkURLLoaderFactory::~NonNetworkURLLoaderFactory() = default;

void NonNetworkURLLoaderFactory::BindRequest(
    mojom::URLLoaderFactoryRequest request) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  BrowserThread::PostTask(BrowserThread::IO, FROM_HERE,
                          base::BindOnce(&NonNetworkURLLoaderFactory::BindOnIO,
                                         this, std::move(request)));
}

void NonNetworkURLLoaderFactory::CreateLoaderAndStart(
    mojom::URLLoaderRequest loader,
    int32_t routing_id,
    int32_t request_id,
    uint32_t options,
    const ResourceRequest& request,
    mojom::URLLoaderClientPtr client,
    const net::MutableNetworkTrafficAnnotationTag& traffic_annotation) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);
  if (request.url.SchemeIsBlob()) {
    std::unique_ptr<storage::BlobDataHandle> blob_handle;
    if (blob_storage_context_) {
      blob_handle =
          blob_storage_context_->GetBlobDataFromPublicURL(request.url);
    }
    BlobURLLoader::CreateAndStart(std::move(loader), request, std::move(client),
                                  std::move(blob_handle),
                                  file_system_context_.get());
  } else {
    // TODO(rockot): Implement support for protocol handlers.
    NOTIMPLEMENTED() << "Only blob:// scheme is currently supported. Unable to "
                     << "fetch " << request.url.spec();
  }
}

void NonNetworkURLLoaderFactory::Clone(mojom::URLLoaderFactoryRequest request) {
  BindRequest(std::move(request));
}

void NonNetworkURLLoaderFactory::InitializeOnIO(
    BlobContextGetter blob_storage_context_getter) {
  blob_storage_context_ = std::move(blob_storage_context_getter).Run();
}

void NonNetworkURLLoaderFactory::BindOnIO(
    mojom::URLLoaderFactoryRequest request) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);
  bindings_.AddBinding(this, std::move(request));
}

}  // namespace content
