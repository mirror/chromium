// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/cache_storage/cache_storage_dispatcher_host.h"

#include <stddef.h>
#include <utility>

#include "base/bind.h"
#include "base/logging.h"
#include "base/macros.h"
#include "base/strings/string16.h"
#include "base/strings/utf_string_conversions.h"
#include "base/threading/thread_task_runner_handle.h"
#include "base/time/time.h"
#include "base/trace_event/trace_event.h"
#include "content/browser/bad_message.h"
#include "content/browser/cache_storage/cache_storage_cache.h"
#include "content/browser/cache_storage/cache_storage_cache_handle.h"
#include "content/browser/cache_storage/cache_storage_cache_impl.h"
#include "content/browser/cache_storage/cache_storage_context_impl.h"
#include "content/browser/cache_storage/cache_storage_manager.h"
#include "content/common/cache_storage/cache_storage_messages.h"
#include "content/common/service_worker/service_worker_cache_storage_struct_traits.h"
#include "content/public/browser/content_browser_client.h"
#include "content/public/common/origin_util.h"
#include "storage/browser/blob/blob_data_handle.h"
#include "third_party/WebKit/public/platform/modules/cache_storage/cache_storage.mojom.h"
#include "url/gurl.h"
#include "url/origin.h"

using blink::mojom::CacheStorageError;

namespace content {

namespace {

const uint32_t kCacheFilteredMessageClasses[] = {CacheStorageMsgStart};
const int32_t kCachePreservationSeconds = 5;

bool OriginCanAccessCacheStorage(const url::Origin& origin) {
  return !origin.unique() && IsOriginSecure(origin.GetURL());
}

void StopPreservingCache(CacheStorageCacheHandle cache_handle) {}

// TODO(cmumford): Rename after deleting legacy IPC methods.
void OnCacheStorageHasCallbackMojo(
    blink::mojom::CacheStorage::HasCallback callback,
    bool has_cache,
    CacheStorageError error) {
  if (error != CacheStorageError::kSuccess) {
    std::move(callback).Run(blink::mojom::HasResult::NewStatus(error));
    return;
  }
  if (!has_cache) {
    std::move(callback).Run(blink::mojom::HasResult::NewStatus(
        CacheStorageError::kErrorCacheNameNotFound));
    return;
  }

  std::move(callback).Run(blink::mojom::HasResult::NewHasCache(has_cache));
}

void OnCacheStorageDeleteCallbackMojo(
    blink::mojom::CacheStorage::DeleteCallback callback,
    bool deleted,
    CacheStorageError error) {
  CacheStorageError e;
  if (!deleted || error != CacheStorageError::kSuccess)
    e = error;
  else
    e = CacheStorageError::kSuccess;
  std::move(callback).Run(e);
}

void OnCacheStorageKeysCallbackMojo(
    blink::mojom::CacheStorage::KeysCallback callback,
    const CacheStorageIndex& cache_index) {
  std::vector<std::string> names;
  for (const auto& metadata : cache_index.ordered_cache_metadata())
    names.push_back(metadata.name);
  std::move(callback).Run(std::move(names));
}

}  // namespace

CacheStorageDispatcherHost::CacheStorageDispatcherHost()
    : BrowserMessageFilter(kCacheFilteredMessageClasses,
                           arraysize(kCacheFilteredMessageClasses)) {}

CacheStorageDispatcherHost::~CacheStorageDispatcherHost() {
}

void CacheStorageDispatcherHost::Init(CacheStorageContextImpl* context) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  BrowserThread::PostTask(
      BrowserThread::IO, FROM_HERE,
      base::BindOnce(&CacheStorageDispatcherHost::CreateCacheListener, this,
                     base::RetainedRef(context)));
}

void CacheStorageDispatcherHost::OnDestruct() const {
  BrowserThread::DeleteOnIOThread::Destruct(this);
}

bool CacheStorageDispatcherHost::OnMessageReceived(
    const IPC::Message& message) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);

  bool handled = true;
  IPC_BEGIN_MESSAGE_MAP(CacheStorageDispatcherHost, message)
  IPC_MESSAGE_HANDLER(CacheStorageHostMsg_CacheStorageHas, OnCacheStorageHas)
  IPC_MESSAGE_HANDLER(CacheStorageHostMsg_CacheStorageOpen, OnCacheStorageOpen)
  IPC_MESSAGE_HANDLER(CacheStorageHostMsg_CacheStorageDelete,
                      OnCacheStorageDelete)
  IPC_MESSAGE_HANDLER(CacheStorageHostMsg_CacheStorageKeys, OnCacheStorageKeys)
  IPC_MESSAGE_HANDLER(CacheStorageHostMsg_CacheStorageMatch,
                      OnCacheStorageMatch)
  IPC_MESSAGE_HANDLER(CacheStorageHostMsg_CacheMatch, OnCacheMatch)
  IPC_MESSAGE_HANDLER(CacheStorageHostMsg_CacheMatchAll, OnCacheMatchAll)
  IPC_MESSAGE_HANDLER(CacheStorageHostMsg_CacheKeys, OnCacheKeys)
  IPC_MESSAGE_HANDLER(CacheStorageHostMsg_CacheBatch, OnCacheBatch)
  IPC_MESSAGE_HANDLER(CacheStorageHostMsg_CacheClosed, OnCacheClosed)
  IPC_MESSAGE_HANDLER(CacheStorageHostMsg_BlobDataHandled, OnBlobDataHandled)
  IPC_MESSAGE_UNHANDLED(handled = false)
  IPC_END_MESSAGE_MAP()

  if (!handled)
    bad_message::ReceivedBadMessage(this, bad_message::CSDH_NOT_RECOGNIZED);
  return handled;
}

void CacheStorageDispatcherHost::CreateCacheListener(
    CacheStorageContextImpl* context) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);
  context_ = context;
}

// TODO(cmumford): Should these strings be a string16?
void CacheStorageDispatcherHost::Has(const url::Origin& origin,
                                     const std::string& cache_name,
                                     HasCallback callback) {
  TRACE_EVENT0("CacheStorage", "CacheStorageDispatcherHost::Has");
  if (!OriginCanAccessCacheStorage(origin)) {
    // TODO(cmumford): Correctly handle a bad message.
    bad_message::ReceivedBadMessage(this, bad_message::CSDH_INVALID_ORIGIN);
    return;
  }
  context_->cache_manager()->HasCache(
      origin.GetURL(), cache_name,
      base::BindOnce(&OnCacheStorageHasCallbackMojo, std::move(callback)));
}

void CacheStorageDispatcherHost::Open(const url::Origin& origin,
                                      const std::string& cache_name,
                                      OpenCallback callback) {
  TRACE_EVENT0("CacheStorage", "CacheStorageDispatcherHost::Open");
  if (!OriginCanAccessCacheStorage(origin)) {
    bad_message::ReceivedBadMessage(this, bad_message::CSDH_INVALID_ORIGIN);
    return;
  }
  context_->cache_manager()->OpenCache(
      origin.GetURL(), cache_name,
      base::BindOnce(
          &CacheStorageDispatcherHost::OnCacheStorageOpenCallbackMojo, this,
          std::move(callback)));
}

void CacheStorageDispatcherHost::Delete(const url::Origin& origin,
                                        const std::string& cache_name,
                                        DeleteCallback callback) {
  TRACE_EVENT0("CacheStorage", "CacheStorageDispatcherHost::Delete");
  if (!OriginCanAccessCacheStorage(origin)) {
    bad_message::ReceivedBadMessage(this, bad_message::CSDH_INVALID_ORIGIN);
    return;
  }
  context_->cache_manager()->DeleteCache(
      origin.GetURL(), cache_name,
      base::BindOnce(&OnCacheStorageDeleteCallbackMojo, std::move(callback)));
}

void CacheStorageDispatcherHost::Keys(const url::Origin& origin,
                                      KeysCallback callback) {
  TRACE_EVENT0("CacheStorage", "CacheStorageDispatcherHost::Keys");
  if (!OriginCanAccessCacheStorage(origin)) {
    bad_message::ReceivedBadMessage(this, bad_message::CSDH_INVALID_ORIGIN);
    return;
  }
  context_->cache_manager()->EnumerateCaches(
      origin.GetURL(),
      base::BindOnce(&OnCacheStorageKeysCallbackMojo, std::move(callback)));
}

void CacheStorageDispatcherHost::Match(
    const url::Origin& origin,
    const content::ServiceWorkerFetchRequest& request,
    const CacheStorageCacheQueryParams& match_params,
    MatchCallback callback) {
  TRACE_EVENT0("CacheStorage", "CacheStorageDispatcherHost::Match");
  if (!OriginCanAccessCacheStorage(origin)) {
    bad_message::ReceivedBadMessage(this, bad_message::CSDH_INVALID_ORIGIN);
    return;
  }
  std::unique_ptr<ServiceWorkerFetchRequest> scoped_request(
      new ServiceWorkerFetchRequest(request.url, request.method,
                                    request.headers, request.referrer,
                                    request.is_reload));

  if (match_params.cache_name.is_null()) {
    context_->cache_manager()->MatchAllCaches(
        origin.GetURL(), std::move(scoped_request), match_params,
        base::BindOnce(
            &CacheStorageDispatcherHost::OnCacheStorageMatchCallbackMojo, this,
            std::move(callback)));
    return;
  }
  context_->cache_manager()->MatchCache(
      origin.GetURL(), base::UTF16ToUTF8(match_params.cache_name.string()),
      std::move(scoped_request), match_params,
      base::BindOnce(
          &CacheStorageDispatcherHost::OnCacheStorageMatchCallbackMojo, this,
          std::move(callback)));
}

void CacheStorageDispatcherHost::OnCacheStorageHas(
    int thread_id,
    int request_id,
    const url::Origin& origin,
    const base::string16& cache_name) {
  TRACE_EVENT0("CacheStorage", "CacheStorageDispatcherHost::OnCacheStorageHas");
  if (!OriginCanAccessCacheStorage(origin)) {
    bad_message::ReceivedBadMessage(this, bad_message::CSDH_INVALID_ORIGIN);
    return;
  }
  context_->cache_manager()->HasCache(
      origin.GetURL(), base::UTF16ToUTF8(cache_name),
      base::BindOnce(&CacheStorageDispatcherHost::OnCacheStorageHasCallback,
                     this, thread_id, request_id));
}

void CacheStorageDispatcherHost::OnCacheStorageOpen(
    int thread_id,
    int request_id,
    const url::Origin& origin,
    const base::string16& cache_name) {
  TRACE_EVENT0("CacheStorage",
               "CacheStorageDispatcherHost::OnCacheStorageOpen");
  if (!OriginCanAccessCacheStorage(origin)) {
    bad_message::ReceivedBadMessage(this, bad_message::CSDH_INVALID_ORIGIN);
    return;
  }
  context_->cache_manager()->OpenCache(
      origin.GetURL(), base::UTF16ToUTF8(cache_name),
      base::BindOnce(&CacheStorageDispatcherHost::OnCacheStorageOpenCallback,
                     this, thread_id, request_id));
}

void CacheStorageDispatcherHost::OnCacheStorageDelete(
    int thread_id,
    int request_id,
    const url::Origin& origin,
    const base::string16& cache_name) {
  TRACE_EVENT0("CacheStorage",
               "CacheStorageDispatcherHost::OnCacheStorageDelete");
  if (!OriginCanAccessCacheStorage(origin)) {
    bad_message::ReceivedBadMessage(this, bad_message::CSDH_INVALID_ORIGIN);
    return;
  }
  context_->cache_manager()->DeleteCache(
      origin.GetURL(), base::UTF16ToUTF8(cache_name),
      base::BindOnce(&CacheStorageDispatcherHost::OnCacheStorageDeleteCallback,
                     this, thread_id, request_id));
}

void CacheStorageDispatcherHost::OnCacheStorageKeys(int thread_id,
                                                    int request_id,
                                                    const url::Origin& origin) {
  TRACE_EVENT0("CacheStorage",
               "CacheStorageDispatcherHost::OnCacheStorageKeys");
  if (!OriginCanAccessCacheStorage(origin)) {
    bad_message::ReceivedBadMessage(this, bad_message::CSDH_INVALID_ORIGIN);
    return;
  }
  context_->cache_manager()->EnumerateCaches(
      origin.GetURL(),
      base::BindOnce(&CacheStorageDispatcherHost::OnCacheStorageKeysCallback,
                     this, thread_id, request_id));
}

void CacheStorageDispatcherHost::OnCacheStorageMatch(
    int thread_id,
    int request_id,
    const url::Origin& origin,
    const ServiceWorkerFetchRequest& request,
    const CacheStorageCacheQueryParams& match_params) {
  TRACE_EVENT0("CacheStorage",
               "CacheStorageDispatcherHost::OnCacheStorageMatch");
  if (!OriginCanAccessCacheStorage(origin)) {
    bad_message::ReceivedBadMessage(this, bad_message::CSDH_INVALID_ORIGIN);
    return;
  }
  std::unique_ptr<ServiceWorkerFetchRequest> scoped_request(
      new ServiceWorkerFetchRequest(request.url, request.method,
                                    request.headers, request.referrer,
                                    request.is_reload));

  if (match_params.cache_name.is_null()) {
    context_->cache_manager()->MatchAllCaches(
        origin.GetURL(), std::move(scoped_request), match_params,
        base::BindOnce(&CacheStorageDispatcherHost::OnCacheStorageMatchCallback,
                       this, thread_id, request_id));
    return;
  }
  context_->cache_manager()->MatchCache(
      origin.GetURL(), base::UTF16ToUTF8(match_params.cache_name.string()),
      std::move(scoped_request), match_params,
      base::BindOnce(&CacheStorageDispatcherHost::OnCacheStorageMatchCallback,
                     this, thread_id, request_id));
}

void CacheStorageDispatcherHost::OnCacheMatch(
    int thread_id,
    int request_id,
    int cache_id,
    const ServiceWorkerFetchRequest& request,
    const CacheStorageCacheQueryParams& match_params) {
  IDToCacheMap::iterator it = id_to_cache_map_.find(cache_id);
  if (it == id_to_cache_map_.end() || !it->second.value()) {
    Send(new CacheStorageMsg_CacheMatchError(
        thread_id, request_id, CacheStorageError::kErrorNotFound));
    return;
  }

  CacheStorageCache* cache = it->second.value();
  std::unique_ptr<ServiceWorkerFetchRequest> scoped_request(
      new ServiceWorkerFetchRequest(request.url, request.method,
                                    request.headers, request.referrer,
                                    request.is_reload));
  cache->Match(
      std::move(scoped_request), match_params,
      base::BindOnce(&CacheStorageDispatcherHost::OnCacheMatchCallback, this,
                     thread_id, request_id, base::Passed(it->second.Clone())));
}

void CacheStorageDispatcherHost::OnCacheMatchAll(
    int thread_id,
    int request_id,
    int cache_id,
    const ServiceWorkerFetchRequest& request,
    const CacheStorageCacheQueryParams& match_params) {
  IDToCacheMap::iterator it = id_to_cache_map_.find(cache_id);
  if (it == id_to_cache_map_.end() || !it->second.value()) {
    Send(new CacheStorageMsg_CacheMatchError(
        thread_id, request_id, CacheStorageError::kErrorNotFound));
    return;
  }

  CacheStorageCache* cache = it->second.value();
  if (request.url.is_empty()) {
    cache->MatchAll(
        std::unique_ptr<ServiceWorkerFetchRequest>(), match_params,
        base::BindOnce(&CacheStorageDispatcherHost::OnCacheMatchAllCallback,
                       this, thread_id, request_id,
                       base::Passed(it->second.Clone())));
    return;
  }

  std::unique_ptr<ServiceWorkerFetchRequest> scoped_request(
      new ServiceWorkerFetchRequest(request.url, request.method,
                                    request.headers, request.referrer,
                                    request.is_reload));
  if (match_params.ignore_search) {
    cache->MatchAll(
        std::move(scoped_request), match_params,
        base::BindOnce(&CacheStorageDispatcherHost::OnCacheMatchAllCallback,
                       this, thread_id, request_id,
                       base::Passed(it->second.Clone())));
    return;
  }
  cache->Match(
      std::move(scoped_request), match_params,
      base::BindOnce(
          &CacheStorageDispatcherHost::OnCacheMatchAllCallbackAdapter, this,
          thread_id, request_id, base::Passed(it->second.Clone())));
}

void CacheStorageDispatcherHost::OnCacheKeys(
    int thread_id,
    int request_id,
    int cache_id,
    const ServiceWorkerFetchRequest& request,
    const CacheStorageCacheQueryParams& match_params) {
  IDToCacheMap::iterator it = id_to_cache_map_.find(cache_id);
  if (it == id_to_cache_map_.end() || !it->second.value()) {
    Send(new CacheStorageMsg_CacheKeysError(thread_id, request_id,
                                            CacheStorageError::kErrorNotFound));
    return;
  }

  CacheStorageCache* cache = it->second.value();
  std::unique_ptr<ServiceWorkerFetchRequest> request_ptr(
      new ServiceWorkerFetchRequest(request.url, request.method,
                                    request.headers, request.referrer,
                                    request.is_reload));
  cache->Keys(
      std::move(request_ptr), match_params,
      base::BindOnce(&CacheStorageDispatcherHost::OnCacheKeysCallback, this,
                     thread_id, request_id, base::Passed(it->second.Clone())));
}

void CacheStorageDispatcherHost::OnCacheBatch(
    int thread_id,
    int request_id,
    int cache_id,
    const std::vector<CacheStorageBatchOperation>& operations) {
  IDToCacheMap::iterator it = id_to_cache_map_.find(cache_id);
  if (it == id_to_cache_map_.end() || !it->second.value()) {
    Send(new CacheStorageMsg_CacheBatchError(
        thread_id, request_id, CacheStorageError::kErrorNotFound));
    return;
  }

  CacheStorageCache* cache = it->second.value();
  cache->BatchOperation(
      operations,
      base::BindOnce(&CacheStorageDispatcherHost::OnCacheBatchCallback, this,
                     thread_id, request_id, base::Passed(it->second.Clone())));
}

void CacheStorageDispatcherHost::OnCacheClosed(int cache_id) {
  DropCacheReference(cache_id);
}

void CacheStorageDispatcherHost::OnBlobDataHandled(const std::string& uuid) {
  DropBlobDataHandle(uuid);
}

void CacheStorageDispatcherHost::OnCacheStorageHasCallback(
    int thread_id,
    int request_id,
    bool has_cache,
    CacheStorageError error) {
  if (error != CacheStorageError::kSuccess) {
    Send(
        new CacheStorageMsg_CacheStorageHasError(thread_id, request_id, error));
    return;
  }
  if (!has_cache) {
    Send(new CacheStorageMsg_CacheStorageHasError(
        thread_id, request_id, CacheStorageError::kErrorNotFound));
    return;
  }
  Send(new CacheStorageMsg_CacheStorageHasSuccess(thread_id, request_id));
}

void CacheStorageDispatcherHost::OnCacheStorageOpenCallbackMojo(
    blink::mojom::CacheStorage::OpenCallback callback,
    CacheStorageCacheHandle cache_handle,
    CacheStorageError error) {
  if (error != CacheStorageError::kSuccess) {
    std::move(callback).Run(blink::mojom::OpenResult::NewStatus(error));
    return;
  }

  // Hang on to the cache for a few seconds. This way if the user quickly closes
  // and reopens it the cache backend won't have to be reinitialized.
  base::ThreadTaskRunnerHandle::Get()->PostDelayedTask(
      FROM_HERE, base::BindOnce(&StopPreservingCache, cache_handle.Clone()),
      base::TimeDelta::FromSeconds(kCachePreservationSeconds));

  ::blink::mojom::CacheStorageCacheAssociatedPtrInfo ptr_info;
  auto request = mojo::MakeRequest(&ptr_info);
  auto cache_impl =
      base::MakeUnique<CacheStorageCacheImpl>(std::move(cache_handle), this);
  cache_bindings_.AddBinding(std::move(cache_impl), std::move(request));

  std::move(callback).Run(
      blink::mojom::OpenResult::NewCache(std::move(ptr_info)));
}

void CacheStorageDispatcherHost::OnCacheStorageOpenCallback(
    int thread_id,
    int request_id,
    CacheStorageCacheHandle cache_handle,
    CacheStorageError error) {
  if (error != CacheStorageError::kSuccess) {
    Send(new CacheStorageMsg_CacheStorageOpenError(thread_id, request_id,
                                                   error));
    return;
  }

  // Hang on to the cache for a few seconds. This way if the user quickly closes
  // and reopens it the cache backend won't have to be reinitialized.
  base::ThreadTaskRunnerHandle::Get()->PostDelayedTask(
      FROM_HERE,
      base::BindOnce(&StopPreservingCache, base::Passed(cache_handle.Clone())),
      base::TimeDelta::FromSeconds(kCachePreservationSeconds));

  CacheID cache_id = StoreCacheReference(std::move(cache_handle));
  Send(new CacheStorageMsg_CacheStorageOpenSuccess(thread_id, request_id,
                                                   cache_id));
}

void CacheStorageDispatcherHost::OnCacheStorageDeleteCallback(
    int thread_id,
    int request_id,
    bool deleted,
    CacheStorageError error) {
  if (!deleted || error != CacheStorageError::kSuccess) {
    Send(new CacheStorageMsg_CacheStorageDeleteError(thread_id, request_id,
                                                     error));
    return;
  }
  Send(new CacheStorageMsg_CacheStorageDeleteSuccess(thread_id, request_id));
}

void CacheStorageDispatcherHost::OnCacheStorageKeysCallback(
    int thread_id,
    int request_id,
    const CacheStorageIndex& cache_index) {
  std::vector<base::string16> string16s;
  for (const auto& metadata : cache_index.ordered_cache_metadata())
    string16s.push_back(base::UTF8ToUTF16(metadata.name));
  Send(new CacheStorageMsg_CacheStorageKeysSuccess(thread_id, request_id,
                                                   string16s));
}

void CacheStorageDispatcherHost::OnCacheStorageMatchCallback(
    int thread_id,
    int request_id,
    CacheStorageError error,
    std::unique_ptr<ServiceWorkerResponse> response,
    std::unique_ptr<storage::BlobDataHandle> blob_data_handle) {
  if (error != CacheStorageError::kSuccess) {
    Send(new CacheStorageMsg_CacheStorageMatchError(thread_id, request_id,
                                                    error));
    return;
  }

  if (blob_data_handle)
    StoreBlobDataHandle(*blob_data_handle);

  Send(new CacheStorageMsg_CacheStorageMatchSuccess(thread_id, request_id,
                                                    *response));
}

void CacheStorageDispatcherHost::OnCacheStorageMatchCallbackMojo(
    blink::mojom::CacheStorage::MatchCallback callback,
    CacheStorageError error,
    std::unique_ptr<ServiceWorkerResponse> response,
    std::unique_ptr<storage::BlobDataHandle> blob_data_handle) {
  if (error != CacheStorageError::kSuccess) {
    std::move(callback).Run(blink::mojom::MatchResult::NewStatus(error));
    return;
  }

  if (blob_data_handle)
    StoreBlobDataHandle(*blob_data_handle);

  std::move(callback).Run(blink::mojom::MatchResult::NewResponse(*response));
}

void CacheStorageDispatcherHost::OnCacheMatchCallback(
    int thread_id,
    int request_id,
    CacheStorageCacheHandle cache_handle,
    CacheStorageError error,
    std::unique_ptr<ServiceWorkerResponse> response,
    std::unique_ptr<storage::BlobDataHandle> blob_data_handle) {
  if (error != CacheStorageError::kSuccess) {
    Send(new CacheStorageMsg_CacheMatchError(thread_id, request_id, error));
    return;
  }

  if (blob_data_handle)
    StoreBlobDataHandle(*blob_data_handle);

  Send(new CacheStorageMsg_CacheMatchSuccess(thread_id, request_id, *response));
}

void CacheStorageDispatcherHost::OnCacheMatchAllCallbackAdapter(
    int thread_id,
    int request_id,
    CacheStorageCacheHandle cache_handle,
    CacheStorageError error,
    std::unique_ptr<ServiceWorkerResponse> response,
    std::unique_ptr<storage::BlobDataHandle> blob_data_handle) {
  std::unique_ptr<CacheStorageCache::Responses> responses(
      new CacheStorageCache::Responses);
  std::unique_ptr<CacheStorageCache::BlobDataHandles> blob_data_handles(
      new CacheStorageCache::BlobDataHandles);
  if (error == CacheStorageError::kSuccess) {
    DCHECK(response);
    responses->push_back(*response);
    if (blob_data_handle)
      blob_data_handles->push_back(std::move(blob_data_handle));
  }
  OnCacheMatchAllCallback(thread_id, request_id, std::move(cache_handle), error,
                          std::move(responses), std::move(blob_data_handles));
}

void CacheStorageDispatcherHost::OnCacheMatchAllCallback(
    int thread_id,
    int request_id,
    CacheStorageCacheHandle cache_handle,
    CacheStorageError error,
    std::unique_ptr<CacheStorageCache::Responses> responses,
    std::unique_ptr<CacheStorageCache::BlobDataHandles> blob_data_handles) {
  if (error != CacheStorageError::kSuccess &&
      error != CacheStorageError::kErrorNotFound) {
    Send(new CacheStorageMsg_CacheMatchAllError(thread_id, request_id, error));
    return;
  }

  for (const auto& handle : *blob_data_handles) {
    if (handle)
      StoreBlobDataHandle(*handle);
  }

  Send(new CacheStorageMsg_CacheMatchAllSuccess(thread_id, request_id,
                                                *responses));
}

void CacheStorageDispatcherHost::OnCacheKeysCallback(
    int thread_id,
    int request_id,
    CacheStorageCacheHandle cache_handle,
    CacheStorageError error,
    std::unique_ptr<CacheStorageCache::Requests> requests) {
  if (error != CacheStorageError::kSuccess) {
    Send(new CacheStorageMsg_CacheKeysError(thread_id, request_id, error));
    return;
  }

  Send(new CacheStorageMsg_CacheKeysSuccess(thread_id, request_id, *requests));
}

void CacheStorageDispatcherHost::OnCacheBatchCallback(
    int thread_id,
    int request_id,
    CacheStorageCacheHandle cache_handle,
    CacheStorageError error) {
  if (error != CacheStorageError::kSuccess) {
    Send(new CacheStorageMsg_CacheBatchError(thread_id, request_id, error));
    return;
  }

  Send(new CacheStorageMsg_CacheBatchSuccess(thread_id, request_id));
}

CacheStorageDispatcherHost::CacheID
CacheStorageDispatcherHost::StoreCacheReference(
    CacheStorageCacheHandle cache_handle) {
  int cache_id = next_cache_id_++;
  id_to_cache_map_[cache_id] = std::move(cache_handle);
  return cache_id;
}

void CacheStorageDispatcherHost::DropCacheReference(CacheID cache_id) {
  id_to_cache_map_.erase(cache_id);
}

void CacheStorageDispatcherHost::StoreBlobDataHandle(
    const storage::BlobDataHandle& blob_data_handle) {
  std::pair<UUIDToBlobDataHandleList::iterator, bool> rv =
      blob_handle_store_.insert(std::make_pair(
          blob_data_handle.uuid(), std::list<storage::BlobDataHandle>()));
  rv.first->second.push_front(storage::BlobDataHandle(blob_data_handle));
}

void CacheStorageDispatcherHost::DropBlobDataHandle(const std::string& uuid) {
  UUIDToBlobDataHandleList::iterator it = blob_handle_store_.find(uuid);
  if (it == blob_handle_store_.end())
    return;
  DCHECK(!it->second.empty());
  it->second.pop_front();
  if (it->second.empty())
    blob_handle_store_.erase(it);
}

void CacheStorageDispatcherHost::AddBinding(
    blink::mojom::CacheStorageAssociatedRequest request) {
  bindings_.AddBinding(this, std::move(request));
}

void CacheStorageDispatcherHost::AddCacheBinding(
    std::unique_ptr<blink::mojom::CacheStorageCache> cache,
    blink::mojom::CacheStorageCacheAssociatedRequest request) {
  cache_bindings_.AddBinding(std::move(cache), std::move(request));
}

}  // namespace content
