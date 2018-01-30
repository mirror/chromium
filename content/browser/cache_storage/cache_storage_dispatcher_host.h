// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_CACHE_STORAGE_CACHE_STORAGE_DISPATCHER_HOST_H_
#define CONTENT_BROWSER_CACHE_STORAGE_CACHE_STORAGE_DISPATCHER_HOST_H_

#include <stdint.h>

#include <list>
#include <map>
#include <memory>
#include <string>
#include <vector>

#include "base/macros.h"
#include "content/browser/bad_message.h"
#include "content/browser/cache_storage/cache_storage.h"
#include "content/browser/cache_storage/cache_storage_index.h"
#include "content/public/browser/browser_associated_interface.h"
#include "content/public/browser/browser_message_filter.h"
#include "mojo/public/cpp/bindings/associated_binding_set.h"
#include "mojo/public/cpp/bindings/strong_associated_binding_set.h"

namespace url {
class Origin;
}

namespace content {

class CacheStorageContextImpl;

// Handles Cache Storage related messages sent to the browser process from
// child processes. One host instance exists per child process. All
// messages are processed on the IO thread.
class CONTENT_EXPORT CacheStorageDispatcherHost
    : public BrowserMessageFilter,
      public blink::mojom::CacheStorage {
 public:
  CacheStorageDispatcherHost();

  // Runs on UI thread.
  void Init(CacheStorageContextImpl* context);

  // Binds Mojo request to this instance, should be called on IO thread.
  void AddBinding(blink::mojom::CacheStorageRequest request);

  // BrowserMessageFilter implementation
  void OnDestruct() const override;
  bool OnMessageReceived(const IPC::Message& message) override;

 private:
  // Friends to allow OnDestruct() delegation
  friend class BrowserThread;
  friend class base::DeleteHelper<CacheStorageDispatcherHost>;

  typedef int32_t CacheID;  // TODO(jkarlin): Bump to 64 bit.
  typedef std::map<CacheID, CacheStorageCacheHandle> IDToCacheMap;
  typedef std::map<std::string, std::list<storage::BlobDataHandle>>
      UUIDToBlobDataHandleList;

  ~CacheStorageDispatcherHost() override;

  // Called by Init() on IO thread.
  void CreateCacheListener(CacheStorageContextImpl* context);

  // The message receiver functions for the CacheStorage API:
  void OnCacheStorageOpen(int thread_id,
                          int request_id,
                          const url::Origin& origin,
                          const base::string16& cache_name);
  void OnCacheStorageMatch(int thread_id,
                           int request_id,
                           const url::Origin& origin,
                           const ServiceWorkerFetchRequest& request,
                           const CacheStorageCacheQueryParams& match_params);

  // Mojo CacheStorage Interface implementation:
  void Keys(const url::Origin& origin,
            blink::mojom::CacheStorage::KeysCallback callback) override;

  void Delete(const url::Origin& origin,
              const base::string16& cache_name,
              blink::mojom::CacheStorage::DeleteCallback callback) override;
  void Has(const url::Origin& origin,
           const base::string16& cache_name,
           blink::mojom::CacheStorage::HasCallback callback) override;
  void Match(const url::Origin& origin,
             const content::ServiceWorkerFetchRequest& request,
             const content::CacheStorageCacheQueryParams& match_params,
             blink::mojom::CacheStorage::MatchCallback callback) override;

  // Callbacks used by Mojo implementation:
  void OnCacheStorageKeysCallback(
      blink::mojom::CacheStorage::KeysCallback callback,
      const CacheStorageIndex& cache_index);
  void OnCacheStorageDeleteCallback(
      blink::mojom::CacheStorage::DeleteCallback callback,
      blink::mojom::CacheStorageError error);
  void OnCacheStorageHasCallback(
      blink::mojom::CacheStorage::HasCallback callback,
      bool has_cache,
      blink::mojom::CacheStorageError error);
  void OnCacheStorageMatchCallback(
      blink::mojom::CacheStorage::MatchCallback callback,
      blink::mojom::CacheStorageError error,
      std::unique_ptr<ServiceWorkerResponse> response,
      std::unique_ptr<storage::BlobDataHandle> blob_data_handle);

  // The message receiver functions for the Cache API:
  void OnCacheMatch(int thread_id,
                    int request_id,
                    int cache_id,
                    const ServiceWorkerFetchRequest& request,
                    const CacheStorageCacheQueryParams& match_params);
  void OnCacheKeys(int thread_id,
                   int request_id,
                   int cache_id,
                   const ServiceWorkerFetchRequest& request,
                   const CacheStorageCacheQueryParams& match_params);
  void OnCacheBatch(int thread_id,
                    int request_id,
                    int cache_id,
                    const std::vector<CacheStorageBatchOperation>& operations);
  void OnCacheClosed(int cache_id);
  void OnBlobDataHandled(const std::string& uuid);

  // CacheStorageManager callbacks
  void OnCacheStorageOpenCallback(int thread_id,
                                  int request_id,
                                  CacheStorageCacheHandle cache_handle,
                                  blink::mojom::CacheStorageError error);

  // Cache callbacks.
  void OnCacheMatchCallback(
      int thread_id,
      int request_id,
      CacheStorageCacheHandle cache_handle,
      blink::mojom::CacheStorageError error,
      std::unique_ptr<ServiceWorkerResponse> response,
      std::unique_ptr<storage::BlobDataHandle> blob_data_handle);
  void OnCacheMatchAllCallbackAdapter(
      int thread_id,
      int request_id,
      CacheStorageCacheHandle cache_handle,
      blink::mojom::CacheStorageError error,
      std::unique_ptr<ServiceWorkerResponse> response,
      std::unique_ptr<storage::BlobDataHandle> blob_data_handle);
  void OnCacheMatchAllCallback(
      int thread_id,
      int request_id,
      CacheStorageCacheHandle cache_handle,
      blink::mojom::CacheStorageError error,
      std::vector<ServiceWorkerResponse> responses,
      std::unique_ptr<CacheStorageCache::BlobDataHandles> blob_data_handles);
  void OnCacheMatchAll(int thread_id,
                       int request_id,
                       int cache_id,
                       const ServiceWorkerFetchRequest& request,
                       const CacheStorageCacheQueryParams& match_params);
  void OnCacheKeysCallback(
      int thread_id,
      int request_id,
      CacheStorageCacheHandle cache_handle,
      blink::mojom::CacheStorageError error,
      std::unique_ptr<CacheStorageCache::Requests> requests);
  void OnCacheBatchCallback(int thread_id,
                            int request_id,
                            CacheStorageCacheHandle cache_handle,
                            blink::mojom::CacheStorageError error);

  // Called when a bad message is detected while executing operations.
  void OnBadMessage(bad_message::BadMessageReason reason);

  // Hangs onto a cache handle. Returns a unique cache_id. Call
  // DropCacheReference when the reference is no longer needed.
  CacheID StoreCacheReference(CacheStorageCacheHandle cache_handle);
  void DropCacheReference(CacheID cache_id);

  // Stores blob handles while waiting for acknowledgement of receipt from the
  // renderer.
  void StoreBlobDataHandle(const storage::BlobDataHandle& blob_data_handle);
  void DropBlobDataHandle(const std::string& uuid);

  IDToCacheMap id_to_cache_map_;
  CacheID next_cache_id_ = 0;

  UUIDToBlobDataHandleList blob_handle_store_;

  scoped_refptr<CacheStorageContextImpl> context_;
  // TODO(lucmult): MakeStrongBinding once message filter is gone.
  mojo::BindingSet<blink::mojom::CacheStorage> bindings_;

  DISALLOW_COPY_AND_ASSIGN(CacheStorageDispatcherHost);
};

}  // namespace content

#endif  // CONTENT_BROWSER_CACHE_STORAGE_CACHE_STORAGE_DISPATCHER_HOST_H_
