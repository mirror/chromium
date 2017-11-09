// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_CACHE_STORAGE_CACHE_STORAGE_CACHE_IMPL_H_
#define CONTENT_BROWSER_CACHE_STORAGE_CACHE_STORAGE_CACHE_IMPL_H_

#include <memory>

#include "base/memory/weak_ptr.h"
#include "content/browser/cache_storage/cache_storage_cache.h"
#include "content/browser/cache_storage/cache_storage_cache_handle.h"
#include "third_party/WebKit/public/platform/modules/cache_storage/cache_storage.mojom.h"
#include "third_party/WebKit/public/platform/modules/serviceworker/service_worker_cache_storage.mojom.h"

namespace content {

class CacheStorageDispatcherHost;

class CONTENT_EXPORT CacheStorageCacheImpl
    : public blink::mojom::CacheStorageCache {
 public:
  CacheStorageCacheImpl(CacheStorageCacheHandle cache_handle,
                        CacheStorageDispatcherHost* dispatcher_host);
  ~CacheStorageCacheImpl() override;

  // blink::mojom::CacheStorageCache implementation:
  void Match(const url::Origin& origin,
             const ServiceWorkerFetchRequest& request,
             const CacheStorageCacheQueryParams& match_params,
             MatchCallback callback) override;

  void MatchAll(const ServiceWorkerFetchRequest& request,
                const CacheStorageCacheQueryParams& match_params,
                MatchAllCallback callback) override;

  void Keys(const ServiceWorkerFetchRequest& request,
            const CacheStorageCacheQueryParams& match_params,
            KeysCallback callback) override;

  void Batch(const std::vector<CacheStorageBatchOperation>& batch_operations,
             BatchCallback callback) override;

  void BlobDataHandled(const std::string& uuid) override;

 private:
  void OnCacheMatchCallback(
      blink::mojom::CacheStorageCache::MatchCallback callback,
      CacheStorageCacheHandle cache_handle,
      blink::mojom::CacheStorageError error,
      std::unique_ptr<ServiceWorkerResponse> response,
      std::unique_ptr<storage::BlobDataHandle> blob_data_handle);

  void OnCacheMatchAllCallback(
      blink::mojom::CacheStorageCache::MatchAllCallback callback,
      CacheStorageCacheHandle cache_handle,
      blink::mojom::CacheStorageError error,
      std::unique_ptr<content::CacheStorageCache::Responses> responses,
      std::unique_ptr<content::CacheStorageCache::BlobDataHandles>
          blob_data_handles);

  void OnCacheMatchAllCallbackAdapter(
      blink::mojom::CacheStorageCache::MatchAllCallback callback,
      CacheStorageCacheHandle cache_handle,
      blink::mojom::CacheStorageError error,
      std::unique_ptr<ServiceWorkerResponse> response,
      std::unique_ptr<storage::BlobDataHandle> blob_data_handle);

  void OnCacheKeysCallback(
      blink::mojom::CacheStorageCache::KeysCallback callback,
      CacheStorageCacheHandle cache_handle,
      blink::mojom::CacheStorageError error,
      std::unique_ptr<content::CacheStorageCache::Requests> requests);

  void OnCacheBatchCallback(
      blink::mojom::CacheStorageCache::BatchCallback callback,
      CacheStorageCacheHandle cache_handle,
      blink::mojom::CacheStorageError error);

  CacheStorageCacheHandle cache_handle_;

  // Dispatcher host owns this instance and Mojo guarantees will outlive this.
  CacheStorageDispatcherHost* dispatcher_host_;

  base::WeakPtrFactory<CacheStorageCacheImpl> ptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(CacheStorageCacheImpl);
};

}  // namespace content

#endif  // CONTENT_BROWSER_CACHE_STORAGE_CACHE_STORAGE_CACHE_IMPL_H_
