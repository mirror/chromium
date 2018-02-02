// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "cache_impl.h"

#include "content/browser/bad_message.h"
#include "content/browser/cache_storage/cache_storage_dispatcher_host.h"
#include "content/common/cache_storage/cache_storage_struct_traits.h"
#include "mojo/public/cpp/bindings/message.h"
#include "storage/browser/blob/blob_data_handle.h"
#include "third_party/WebKit/public/platform/modules/cache_storage/cache_storage.mojom.h"

using blink::mojom::CacheStorageError;

namespace content {

namespace {
const char kInvalidOriginError[] = "INVALID_ORIGIN";
}  // namespace

CacheImpl::CacheImpl(CacheStorageCacheHandle cache_handle,
                     CacheStorageDispatcherHost* dispatcher_host)
    : cache_handle_(std::move(cache_handle)),
      dispatcher_host_(dispatcher_host),
      ptr_factory_(this) {}

CacheImpl::~CacheImpl() {}

void CacheImpl::Match(const ServiceWorkerFetchRequest& request,
                      const CacheStorageCacheQueryParams& match_params,
                      MatchCallback callback) {
  content::CacheStorageCache* cache = cache_handle_.value();
  if (!cache) {
    std::move(callback).Run(blink::mojom::MatchResult::NewStatus(
        CacheStorageError::kErrorNotFound));
    return;
  }

  std::unique_ptr<ServiceWorkerFetchRequest> scoped_request(
      new ServiceWorkerFetchRequest(request.url, request.method,
                                    request.headers, request.referrer,
                                    request.is_reload));
  cache->Match(std::move(scoped_request), match_params,
               base::BindOnce(&CacheImpl::OnCacheMatchCallback,
                              ptr_factory_.GetWeakPtr(), std::move(callback),
                              cache_handle_.Clone()));
}

void CacheImpl::OnCacheMatchCallback(
    blink::mojom::CacheStorageCache::MatchCallback callback,
    CacheStorageCacheHandle cache_handle,
    CacheStorageError error,
    std::unique_ptr<ServiceWorkerResponse> response,
    std::unique_ptr<storage::BlobDataHandle> blob_data_handle) {
  if (error != CacheStorageError::kSuccess) {
    std::move(callback).Run(blink::mojom::MatchResult::NewStatus(error));
    return;
  }

  if (blob_data_handle)
    dispatcher_host_->StoreBlobDataHandle(*blob_data_handle);

  std::move(callback).Run(blink::mojom::MatchResult::NewResponse(*response));
}

void CacheImpl::MatchAll(const ServiceWorkerFetchRequest& request,
                         const CacheStorageCacheQueryParams& match_params,
                         MatchAllCallback callback) {
  content::CacheStorageCache* cache = cache_handle_.value();
  if (!cache) {
    std::move(callback).Run(blink::mojom::MatchAllResult::NewStatus(
        CacheStorageError::kErrorNotFound));
    return;
  }

  if (request.url.is_empty()) {
    cache->MatchAll(
        std::unique_ptr<ServiceWorkerFetchRequest>(), match_params,
        base::BindOnce(&CacheImpl::OnCacheMatchAllCallback,
                       ptr_factory_.GetWeakPtr(), std::move(callback),
                       base::Passed(cache_handle_.Clone())));
    return;
  }

  std::unique_ptr<ServiceWorkerFetchRequest> scoped_request(
      new ServiceWorkerFetchRequest(request.url, request.method,
                                    request.headers, request.referrer,
                                    request.is_reload));
  if (match_params.ignore_search) {
    cache->MatchAll(
        std::move(scoped_request), match_params,
        base::BindOnce(&CacheImpl::OnCacheMatchAllCallback,
                       ptr_factory_.GetWeakPtr(), std::move(callback),
                       base::Passed(cache_handle_.Clone())));
    return;
  }

  cache->Match(std::move(scoped_request), match_params,
               base::BindOnce(&CacheImpl::OnCacheMatchAllCallbackAdapter,
                              ptr_factory_.GetWeakPtr(), std::move(callback),
                              base::Passed(cache_handle_.Clone())));
}

void CacheImpl::OnCacheMatchAllCallback(
    MatchAllCallback callback,
    CacheStorageCacheHandle cache_handle,
    CacheStorageError error,
    std::vector<ServiceWorkerResponse> responses,
    std::unique_ptr<content::CacheStorageCache::BlobDataHandles>
        blob_data_handles) {
  if (error != CacheStorageError::kSuccess &&
      error != CacheStorageError::kErrorNotFound) {
    std::move(callback).Run(blink::mojom::MatchAllResult::NewStatus(error));
    return;
  }

  for (const auto& handle : *blob_data_handles) {
    if (handle)
      dispatcher_host_->StoreBlobDataHandle(*handle);
  }

  std::move(callback).Run(
      blink::mojom::MatchAllResult::NewResponses(std::move(responses)));
}

void CacheImpl::OnCacheMatchAllCallbackAdapter(
    MatchAllCallback callback,
    CacheStorageCacheHandle cache_handle,
    CacheStorageError error,
    std::unique_ptr<ServiceWorkerResponse> response,
    std::unique_ptr<storage::BlobDataHandle> blob_data_handle) {
  std::vector<ServiceWorkerResponse> responses;
  std::unique_ptr<content::CacheStorageCache::BlobDataHandles>
      blob_data_handles(new content::CacheStorageCache::BlobDataHandles);
  if (error == CacheStorageError::kSuccess) {
    DCHECK(response);
    responses.push_back(*response);
    if (blob_data_handle)
      blob_data_handles->push_back(std::move(blob_data_handle));
  }
  OnCacheMatchAllCallback(std::move(callback), std::move(cache_handle), error,
                          std::move(responses), std::move(blob_data_handles));
}

void CacheImpl::Keys(const ServiceWorkerFetchRequest& request,
                     const CacheStorageCacheQueryParams& match_params,
                     KeysCallback callback) {
  content::CacheStorageCache* cache = cache_handle_.value();
  if (!cache) {
    std::move(callback).Run(blink::mojom::CacheKeysResult::NewStatus(
                                CacheStorageError::kErrorNotFound));
    return;
  }

  std::unique_ptr<ServiceWorkerFetchRequest> request_ptr(
      new ServiceWorkerFetchRequest(request.url, request.method,
                                    request.headers, request.referrer,
                                    request.is_reload));
  cache->Keys(
      std::move(request_ptr), match_params,
      base::BindOnce(&CacheImpl::OnCacheKeysCallback, ptr_factory_.GetWeakPtr(),
                     std::move(callback), base::Passed(cache_handle_.Clone())));
}

void CacheImpl::OnCacheKeysCallback(
    KeysCallback callback,
    CacheStorageCacheHandle cache_handle,
    CacheStorageError error,
    std::unique_ptr<content::CacheStorageCache::Requests> requests) {
  if (error != CacheStorageError::kSuccess) {
    std::move(callback).Run(blink::mojom::CacheKeysResult::NewStatus(error));
    return;
  }

  std::move(callback).Run(blink::mojom::CacheKeysResult::NewKeys(*requests));
}

void CacheImpl::Batch(
    const std::vector<CacheStorageBatchOperation>& batch_operations,
    BatchCallback callback) {
  content::CacheStorageCache* cache = cache_handle_.value();
  if (!cache) {
    std::move(callback).Run(CacheStorageError::kErrorNotFound);
    return;
  }
  cache->BatchOperation(
      batch_operations,
      base::BindOnce(&CacheImpl::OnCacheBatchCallback,
                     ptr_factory_.GetWeakPtr(), std::move(callback),
                     base::Passed(cache_handle_.Clone())),
      base::BindOnce(&CacheImpl::OnBadMessage, ptr_factory_.GetWeakPtr()));
}

void CacheImpl::OnCacheBatchCallback(BatchCallback callback,
                                     CacheStorageCacheHandle cache_handle,
                                     CacheStorageError error) {
  std::move(callback).Run(error);
}

void CacheImpl::BlobDataHandled(const std::string& uuid) {
  dispatcher_host_->DropBlobDataHandle(uuid);
}

void CacheImpl::OnBadMessage(bad_message::BadMessageReason reason) {
  mojo::ReportBadMessage(kInvalidOriginError);
}

}  // namespace content
