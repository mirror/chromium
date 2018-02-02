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
#include "content/browser/cache_storage/cache_impl.h"
#include "content/browser/cache_storage/cache_storage_cache.h"
#include "content/browser/cache_storage/cache_storage_cache_handle.h"
#include "content/browser/cache_storage/cache_storage_context_impl.h"
#include "content/browser/cache_storage/cache_storage_manager.h"
#include "content/common/cache_storage/cache_storage_messages.h"
#include "content/public/browser/content_browser_client.h"
#include "content/public/common/origin_util.h"
#include "mojo/public/cpp/bindings/message.h"
#include "storage/browser/blob/blob_data_handle.h"
#include "third_party/WebKit/public/platform/modules/cache_storage/cache_storage.mojom.h"
#include "url/gurl.h"
#include "url/origin.h"


namespace content {

namespace {

using blink::mojom::CacheStorageError;

const int32_t kCachePreservationSeconds = 5;
const char kInvalidOriginError[] = "INVALID_ORIGIN";

bool OriginCanAccessCacheStorage(const url::Origin& origin) {
  return !origin.unique() && IsOriginSecure(origin.GetURL());
}

void StopPreservingCache(CacheStorageCacheHandle cache_handle) {}

}  // namespace

CacheStorageDispatcherHost::CacheStorageDispatcherHost() {}

CacheStorageDispatcherHost::~CacheStorageDispatcherHost() {}

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

void CacheStorageDispatcherHost::CreateCacheListener(
    CacheStorageContextImpl* context) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);
  context_ = context;
}

void CacheStorageDispatcherHost::Has(
    const url::Origin& origin,
    const base::string16& cache_name,
    blink::mojom::CacheStorage::HasCallback callback) {
  TRACE_EVENT0("CacheStorage", "CacheStorageDispatcherHost::OnCacheStorageHas");
  if (!OriginCanAccessCacheStorage(origin)) {
    bindings_.ReportBadMessage(kInvalidOriginError);
    return;
  }
  context_->cache_manager()->HasCache(
      origin.GetURL(), base::UTF16ToUTF8(cache_name),
      base::BindOnce(&CacheStorageDispatcherHost::OnHasCallback,
                     this, std::move(callback)));
}

void CacheStorageDispatcherHost::Open(
    const url::Origin& origin,
    const base::string16& cache_name,
    blink::mojom::CacheStorage::OpenCallback callback) {
  TRACE_EVENT0("CacheStorage",
               "CacheStorageDispatcherHost::OnCacheStorageOpen");
  if (!OriginCanAccessCacheStorage(origin)) {
    bindings_.ReportBadMessage(kInvalidOriginError);
    return;
  }
  context_->cache_manager()->OpenCache(
      origin.GetURL(), base::UTF16ToUTF8(cache_name),
      base::BindOnce(&CacheStorageDispatcherHost::OnOpenCallback,
                     this, std::move(callback)));
}

void CacheStorageDispatcherHost::Delete(
    const url::Origin& origin,
    const base::string16& cache_name,
    blink::mojom::CacheStorage::DeleteCallback callback) {
  TRACE_EVENT0("CacheStorage",
               "CacheStorageDispatcherHost::OnCacheStorageDelete");
  if (!OriginCanAccessCacheStorage(origin)) {
    bindings_.ReportBadMessage(kInvalidOriginError);
    return;
  }
  context_->cache_manager()->DeleteCache(
      origin.GetURL(), base::UTF16ToUTF8(cache_name),
      base::BindOnce(&CacheStorageDispatcherHost::OnDeleteCallback,
                     this, std::move(callback)));
}

void CacheStorageDispatcherHost::Keys(
    const url::Origin& origin,
    blink::mojom::CacheStorage::KeysCallback callback) {
  TRACE_EVENT0("CacheStorage",
               "CacheStorageDispatcherHost::OnCacheStorageKeys");

  if (!OriginCanAccessCacheStorage(origin)) {
    bindings_.ReportBadMessage(kInvalidOriginError);
    return;
  }
  context_->cache_manager()->EnumerateCaches(
      origin.GetURL(),
      base::BindOnce(&CacheStorageDispatcherHost::OnKeysCallback,
                     this, std::move(callback)));
}

void CacheStorageDispatcherHost::Match(
    const url::Origin& origin,
    const content::ServiceWorkerFetchRequest& request,
    const content::CacheStorageCacheQueryParams& match_params,
    blink::mojom::CacheStorage::MatchCallback callback) {
  TRACE_EVENT0("CacheStorage",
               "CacheStorageDispatcherHost::OnCacheStorageMatch");
  if (!OriginCanAccessCacheStorage(origin)) {
    bindings_.ReportBadMessage(kInvalidOriginError);
    return;
  }
  std::unique_ptr<ServiceWorkerFetchRequest> scoped_request(
      new ServiceWorkerFetchRequest(request.url, request.method,
                                    request.headers, request.referrer,
                                    request.is_reload));

  if (match_params.cache_name.is_null()) {
    context_->cache_manager()->MatchAllCaches(
        origin.GetURL(), std::move(scoped_request), std::move(match_params),
        base::BindOnce(&CacheStorageDispatcherHost::OnMatchCallback,
                       this, std::move(callback)));
    return;
  }
  context_->cache_manager()->MatchCache(
      origin.GetURL(), base::UTF16ToUTF8(match_params.cache_name.string()),
      std::move(scoped_request), std::move(match_params),
      base::BindOnce(&CacheStorageDispatcherHost::OnMatchCallback,
                     this, std::move(callback)));
}

void CacheStorageDispatcherHost::BlobDataHandled(const std::string& uuid) {
  DropBlobDataHandle(uuid);
}

void CacheStorageDispatcherHost::OnHasCallback(
    blink::mojom::CacheStorage::HasCallback callback,
    bool has_cache,
    CacheStorageError error) {
  // Only if cache wasn't found we reinforce the error to be "not found".
  if (!has_cache) {
    std::move(callback).Run(CacheStorageError::kErrorNotFound);
    return;
  }
  std::move(callback).Run(error);
}

void CacheStorageDispatcherHost::OnOpenCallback(
    blink::mojom::CacheStorage::OpenCallback callback,
    CacheStorageCacheHandle cache_handle,
    CacheStorageError error) {
  url::Origin origin = bindings_.dispatch_context();
  if (error != CacheStorageError::kSuccess) {
    std::move(callback).Run(blink::mojom::OpenResult::NewStatus(error));
    return;
  }

  // Hang on to the cache for a few seconds. This way if the user quickly closes
  // and reopens it the cache backend won't have to be reinitialized.
  base::ThreadTaskRunnerHandle::Get()->PostDelayedTask(
      FROM_HERE,
      base::BindOnce(&StopPreservingCache, base::Passed(cache_handle.Clone())),
      base::TimeDelta::FromSeconds(kCachePreservationSeconds));

  blink::mojom::CacheStorageCacheAssociatedPtrInfo ptr_info;
  auto request = mojo::MakeRequest(&ptr_info);
  auto cache_impl = std::make_unique<CacheImpl>(std::move(cache_handle), this);
  cache_bindings_.AddBinding(std::move(cache_impl), std::move(request), origin);

  std::move(callback).Run(
      blink::mojom::OpenResult::NewCache(std::move(ptr_info)));
}

void CacheStorageDispatcherHost::OnDeleteCallback(
    blink::mojom::CacheStorage::DeleteCallback callback,
    CacheStorageError error) {
  // |error| indicates success or error, value CacheStorageError::kSuccess
  // signals a successful operation.
  std::move(callback).Run(error);
}

void CacheStorageDispatcherHost::OnKeysCallback(
    blink::mojom::CacheStorage::KeysCallback callback,
    const CacheStorageIndex& cache_index) {
  std::vector<base::string16> string16s;
  for (const auto& metadata : cache_index.ordered_cache_metadata()) {
    string16s.push_back(base::UTF8ToUTF16(metadata.name));
  }

  std::move(callback).Run(string16s);
}

void CacheStorageDispatcherHost::OnMatchCallback(
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

  std::move(callback).Run(
      blink::mojom::MatchResult::NewResponse(std::move(*response)));
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
    blink::mojom::CacheStorageRequest request,
    const url::Origin& origin) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);
  bindings_.AddBinding(this, std::move(request), origin);
}

}  // namespace content
