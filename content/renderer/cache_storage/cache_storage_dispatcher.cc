// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/renderer/cache_storage/cache_storage_dispatcher.h"

#include <stddef.h>

#include <algorithm>
#include <map>
#include <memory>
#include <string>
#include <utility>

#include "base/lazy_instance.h"
#include "base/logging.h"
#include "base/metrics/histogram_macros.h"
#include "base/strings/utf_string_conversions.h"
#include "base/threading/thread_local.h"
#include "content/child/child_thread_impl.h"
#include "content/child/thread_safe_sender.h"
#include "content/common/cache_storage/cache_storage_messages.h"
#include "content/public/common/content_features.h"
#include "content/public/common/referrer.h"
#include "content/public/common/service_names.mojom.h"
#include "content/public/renderer/render_thread.h"
#include "content/renderer/service_worker/service_worker_type_util.h"
#include "services/service_manager/public/cpp/connector.h"
#include "services/service_manager/public/cpp/interface_provider.h"
#include "storage/common/blob_storage/blob_handle.h"
#include "third_party/WebKit/public/platform/modules/serviceworker/WebServiceWorkerCache.h"
#include "third_party/WebKit/public/platform/modules/serviceworker/WebServiceWorkerRequest.h"
#include "third_party/WebKit/public/platform/modules/serviceworker/WebServiceWorkerResponse.h"
#include "url/origin.h"

namespace content {

using base::TimeTicks;
using blink::mojom::CacheStorageError;
using blink::WebServiceWorkerCacheStorage;
using blink::WebServiceWorkerRequest;
using blink::WebString;

static base::LazyInstance<base::ThreadLocalPointer<CacheStorageDispatcher>>::
    Leaky g_cache_storage_dispatcher_tls = LAZY_INSTANCE_INITIALIZER;

namespace {

CacheStorageDispatcher* const kDeletedCacheStorageDispatcherMarker =
    reinterpret_cast<CacheStorageDispatcher*>(0x1);

ServiceWorkerFetchRequest FetchRequestFromWebRequest(
    const blink::WebServiceWorkerRequest& web_request) {
  ServiceWorkerHeaderMap headers;
  GetServiceWorkerHeaderMapFromWebRequest(web_request, &headers);

  return ServiceWorkerFetchRequest(
      web_request.Url(), web_request.Method().Ascii(), headers,
      Referrer(web_request.ReferrerUrl(), web_request.GetReferrerPolicy()),
      web_request.IsReload());
}

void PopulateWebRequestFromFetchRequest(
    const ServiceWorkerFetchRequest& request,
    blink::WebServiceWorkerRequest* web_request) {
  web_request->SetURL(request.url);
  web_request->SetMethod(WebString::FromASCII(request.method));
  for (ServiceWorkerHeaderMap::const_iterator i = request.headers.begin(),
                                              end = request.headers.end();
       i != end; ++i) {
    web_request->SetHeader(WebString::FromASCII(i->first),
                           WebString::FromASCII(i->second));
  }
  web_request->SetReferrer(WebString::FromASCII(request.referrer.url.spec()),
                           request.referrer.policy);
  web_request->SetIsReload(request.is_reload);
}

blink::WebVector<blink::WebServiceWorkerRequest> WebRequestsFromRequests(
    const std::vector<ServiceWorkerFetchRequest>& requests) {
  blink::WebVector<blink::WebServiceWorkerRequest> web_requests(
      requests.size());
  for (size_t i = 0; i < requests.size(); ++i)
    PopulateWebRequestFromFetchRequest(requests[i], &(web_requests[i]));
  return web_requests;
}

CacheStorageCacheQueryParams QueryParamsFromWebQueryParams(
    const blink::WebServiceWorkerCache::QueryParams& web_query_params) {
  CacheStorageCacheQueryParams query_params;
  query_params.ignore_search = web_query_params.ignore_search;
  query_params.ignore_method = web_query_params.ignore_method;
  query_params.ignore_vary = web_query_params.ignore_vary;
  query_params.cache_name =
      blink::WebString::ToNullableString16(web_query_params.cache_name);
  return query_params;
}

CacheStorageCacheOperationType CacheOperationTypeFromWebCacheOperationType(
    blink::WebServiceWorkerCache::OperationType operation_type) {
  switch (operation_type) {
    case blink::WebServiceWorkerCache::kOperationTypePut:
      return CACHE_STORAGE_CACHE_OPERATION_TYPE_PUT;
    case blink::WebServiceWorkerCache::kOperationTypeDelete:
      return CACHE_STORAGE_CACHE_OPERATION_TYPE_DELETE;
    default:
      return CACHE_STORAGE_CACHE_OPERATION_TYPE_UNDEFINED;
  }
}

CacheStorageBatchOperation BatchOperationFromWebBatchOperation(
    const blink::WebServiceWorkerCache::BatchOperation& web_operation) {
  CacheStorageBatchOperation operation;
  operation.operation_type =
      CacheOperationTypeFromWebCacheOperationType(web_operation.operation_type);
  operation.request = FetchRequestFromWebRequest(web_operation.request);
  operation.response =
      GetServiceWorkerResponseFromWebResponse(web_operation.response);
  operation.match_params =
      QueryParamsFromWebQueryParams(web_operation.match_params);
  return operation;
}

template <typename T>
void ClearCallbacksMapWithErrors(T* callbacks_map) {
  typename T::iterator iter(callbacks_map);
  while (!iter.IsAtEnd()) {
    iter.GetCurrentValue()->OnError(CacheStorageError::kErrorNotFound);
    callbacks_map->Remove(iter.GetCurrentKey());
    iter.Advance();
  }
}

}  // namespace

// The WebCache object is the Chromium side implementation of the Blink
// WebServiceWorkerCache API. Most of its methods delegate directly to the
// ServiceWorkerStorage object, which is able to assign unique IDs as well
// as have a lifetime longer than the requests.
class CacheStorageDispatcher::WebCache : public blink::WebServiceWorkerCache {
 public:
  WebCache(base::WeakPtr<CacheStorageDispatcher> dispatcher,
           blink::mojom::CacheStorageCacheAssociatedPtrInfo cache_ptr_info)
      : dispatcher_(dispatcher) {
    cache_ptr_.Bind(std::move(cache_ptr_info));
  }
  ~WebCache() override {
    ClearCallbacksMapWithErrors(&cache_keys_callbacks_);
    ClearCallbacksMapWithErrors(&cache_match_callbacks_);
    ClearCallbacksMapWithErrors(&cache_match_all_callbacks_);
    ClearCallbacksMapWithErrors(&cache_batch_callbacks_);
  }

  // From blink::WebServiceWorkerCache:
  void DispatchMatch(std::unique_ptr<CacheMatchCallbacks> callbacks,
                     const blink::WebServiceWorkerRequest& request,
                     const QueryParams& query_params) override {
    int request_id = cache_match_callbacks_.Add(std::move(callbacks));
    cache_ptr_->Match(
        FetchRequestFromWebRequest(request),
        QueryParamsFromWebQueryParams(query_params),
        base::BindOnce(&WebCache::CacheMatchCallback, base::Unretained(this),
                       request_id, TimeTicks::Now()));
  }
  void CacheMatchCallback(int request_id,
                          base::TimeTicks start_time,
                          blink::mojom::MatchResultPtr result) {
    blink::WebServiceWorkerCache::CacheMatchCallbacks* callbacks =
        cache_match_callbacks_.Lookup(request_id);

    if (result->is_status() &&
        result->get_status() != CacheStorageError::kSuccess) {
      callbacks->OnError(result->get_status());
    } else {
      blink::WebServiceWorkerResponse web_response;
      dispatcher_->PopulateWebResponseFromResponse(result->get_response(),
                                                   &web_response);

      UMA_HISTOGRAM_TIMES("ServiceWorkerCache.Cache.Match",
                          TimeTicks::Now() - start_time);
      callbacks->OnSuccess(web_response);
    }
    cache_match_callbacks_.Remove(request_id);
  }
  void DispatchMatchAll(std::unique_ptr<CacheWithResponsesCallbacks> callbacks,
                        const blink::WebServiceWorkerRequest& request,
                        const QueryParams& query_params) override {
    int request_id = cache_match_all_callbacks_.Add(std::move(callbacks));
    cache_ptr_->MatchAll(
        FetchRequestFromWebRequest(request),
        QueryParamsFromWebQueryParams(query_params),
        base::BindOnce(&WebCache::CacheMatchAllCallback, base::Unretained(this),
                       request_id, TimeTicks::Now()));
  }
  void CacheMatchAllCallback(int request_id,
                             base::TimeTicks start_time,
                             blink::mojom::MatchAllResultPtr result) {
    blink::WebServiceWorkerCache::CacheWithResponsesCallbacks* callbacks =
        cache_match_all_callbacks_.Lookup(request_id);
    if (result->is_status() &&
        result->get_status() != CacheStorageError::kSuccess) {
      callbacks->OnError(result->get_status());
    } else {
      UMA_HISTOGRAM_TIMES("ServiceWorkerCache.Cache.MatchAll",
                          TimeTicks::Now() - start_time);
      callbacks->OnSuccess(
          dispatcher_->WebResponsesFromResponses(result->get_responses()));
    }
    cache_match_all_callbacks_.Remove(request_id);
  }

  void DispatchKeys(std::unique_ptr<CacheWithRequestsCallbacks> callbacks,
                    const blink::WebServiceWorkerRequest& request,
                    const QueryParams& query_params) override {
    int request_id = cache_keys_callbacks_.Add(std::move(callbacks));
    cache_ptr_->Keys(FetchRequestFromWebRequest(request),
                     QueryParamsFromWebQueryParams(query_params),
                     base::BindOnce(&WebCache::CacheKeysCallback,
                                    base::Unretained(this), request_id));
  }
  void CacheKeysCallback(int request_id,
                         blink::mojom::CacheKeysResultPtr result) {
    blink::WebServiceWorkerCache::CacheWithRequestsCallbacks* callbacks =
        cache_keys_callbacks_.Lookup(request_id);
    if (result->is_status() &&
        result->get_status() != CacheStorageError::kSuccess) {
      callbacks->OnError(result->get_status());
    } else {
      callbacks->OnSuccess(WebRequestsFromRequests(result->get_keys()));
    }
    cache_keys_callbacks_.Remove(request_id);
  }
  void DispatchBatch(
      std::unique_ptr<CacheBatchCallbacks> callbacks,
      const blink::WebVector<BatchOperation>& batch_operations) override {
    int request_id = cache_batch_callbacks_.Add(std::move(callbacks));
    std::vector<CacheStorageBatchOperation> operations;
    operations.reserve(batch_operations.size());
    for (size_t i = 0; i < batch_operations.size(); ++i) {
      operations.push_back(
          BatchOperationFromWebBatchOperation(batch_operations[i]));
    }
    cache_ptr_->Batch(operations, base::BindOnce(&WebCache::BatchCallback,
                                                 base::Unretained(this),
                                                 request_id, TimeTicks::Now()));
  }
  void BatchCallback(int request_id,
                     base::TimeTicks start_time,
                     blink::mojom::CacheStorageError error) {
    blink::WebServiceWorkerCache::CacheBatchCallbacks* callbacks =
        cache_batch_callbacks_.Lookup(request_id);
    if (error == CacheStorageError::kSuccess) {
      UMA_HISTOGRAM_TIMES("ServiceWorkerCache.Cache.Batch",
                          TimeTicks::Now() - start_time);
      callbacks->OnSuccess();
    } else {
      callbacks->OnError(error);
    }
    cache_batch_callbacks_.Remove(request_id);
  }

 private:
  const base::WeakPtr<CacheStorageDispatcher> dispatcher_;

  // Callbacks to send response to js.
  WithRequestsCallbacksMap cache_keys_callbacks_;
  MatchCallbacksMap cache_match_callbacks_;
  WithResponsesCallbacksMap cache_match_all_callbacks_;
  BatchCallbacksMap cache_batch_callbacks_;

  blink::mojom::CacheStorageCacheAssociatedPtr cache_ptr_;
};

CacheStorageDispatcher::CacheStorageDispatcher(
    ThreadSafeSender* thread_safe_sender)
    : thread_safe_sender_(thread_safe_sender), weak_factory_(this) {
  g_cache_storage_dispatcher_tls.Pointer()->Set(this);
}

CacheStorageDispatcher::~CacheStorageDispatcher() {
  ClearCallbacksMapWithErrors(&has_callbacks_);
  ClearCallbacksMapWithErrors(&open_callbacks_);
  ClearCallbacksMapWithErrors(&delete_callbacks_);
  ClearCallbacksMapWithErrors(&keys_callbacks_);
  ClearCallbacksMapWithErrors(&match_callbacks_);


  g_cache_storage_dispatcher_tls.Pointer()->Set(
      kDeletedCacheStorageDispatcherMarker);
}

CacheStorageDispatcher* CacheStorageDispatcher::ThreadSpecificInstance(
    ThreadSafeSender* thread_safe_sender) {
  if (g_cache_storage_dispatcher_tls.Pointer()->Get() ==
      kDeletedCacheStorageDispatcherMarker) {
    NOTREACHED() << "Re-instantiating TLS CacheStorageDispatcher.";
    g_cache_storage_dispatcher_tls.Pointer()->Set(nullptr);
  }
  if (g_cache_storage_dispatcher_tls.Pointer()->Get())
    return g_cache_storage_dispatcher_tls.Pointer()->Get();

  CacheStorageDispatcher* dispatcher =
      new CacheStorageDispatcher(thread_safe_sender);
  if (WorkerThread::GetCurrentId())
    WorkerThread::AddObserver(dispatcher);
  return dispatcher;
}

void CacheStorageDispatcher::WillStopCurrentWorkerThread() {
  delete this;
}

void CacheStorageDispatcher::OnCacheStorageHasCallback(
    int thread_id,
    int request_id,
    base::TimeTicks start_time,
    CacheStorageError result) {
  DCHECK_EQ(thread_id, CurrentWorkerId());
  UMA_HISTOGRAM_TIMES("ServiceWorkerCache.CacheStorage.Has",
                      TimeTicks::Now() - start_time);

  WebServiceWorkerCacheStorage::CacheStorageCallbacks* callbacks =
      has_callbacks_.Lookup(request_id);

  if (result == CacheStorageError::kSuccess) {
    callbacks->OnSuccess();
  } else {
    callbacks->OnError(result);
  }
  has_callbacks_.Remove(request_id);
}

void CacheStorageDispatcher::OnCacheStorageOpenCallback(
    int thread_id,
    int request_id,
    base::TimeTicks start_time,
    blink::mojom::OpenResultPtr result) {
  DCHECK_EQ(thread_id, CurrentWorkerId());
  WebServiceWorkerCacheStorage::CacheStorageWithCacheCallbacks* callbacks =
      open_callbacks_.Lookup(request_id);

  if (result->is_status() &&
      result->get_status() != CacheStorageError::kSuccess) {
    callbacks->OnError(result->get_status());
  } else {
    std::unique_ptr<WebCache> web_cache(new WebCache(
        weak_factory_.GetWeakPtr(), std::move(result->get_cache())));
    UMA_HISTOGRAM_TIMES("ServiceWorkerCache.CacheStorage.Open",
                        TimeTicks::Now() - start_time);
    callbacks->OnSuccess(std::move(web_cache));
  }
  open_callbacks_.Remove(request_id);
}

void CacheStorageDispatcher::CacheStorageDeleteCallback(
    int thread_id,
    int request_id,
    base::TimeTicks start_time,
    CacheStorageError result) {
  DCHECK_EQ(thread_id, CurrentWorkerId());
  UMA_HISTOGRAM_TIMES("ServiceWorkerCache.CacheStorage.Delete",
                      TimeTicks::Now() - start_time);
  WebServiceWorkerCacheStorage::CacheStorageCallbacks* callbacks =
      delete_callbacks_.Lookup(request_id);

  if (result == CacheStorageError::kSuccess) {
    callbacks->OnSuccess();
  } else {
    callbacks->OnError(result);
  }

  delete_callbacks_.Remove(request_id);
}

void CacheStorageDispatcher::KeysCallback(
    int thread_id,
    int request_id,
    base::TimeTicks start_time,
    const std::vector<base::string16>& keys) {
  DCHECK_EQ(thread_id, CurrentWorkerId());
  blink::WebVector<blink::WebString> web_keys(keys.size());
  std::transform(
      keys.begin(), keys.end(), web_keys.begin(),
      [](const base::string16& s) { return WebString::FromUTF16(s); });
  UMA_HISTOGRAM_TIMES("ServiceWorkerCache.CacheStorage.Keys",
                      TimeTicks::Now() - start_time);
  WebServiceWorkerCacheStorage::CacheStorageKeysCallbacks* callbacks =
      keys_callbacks_.Lookup(request_id);
  callbacks->OnSuccess(web_keys);
  keys_callbacks_.Remove(request_id);
}

void CacheStorageDispatcher::OnCacheStorageMatchCallback(
    int thread_id,
    int request_id,
    base::TimeTicks start_time,
    blink::mojom::MatchResultPtr result) {
  DCHECK_EQ(thread_id, CurrentWorkerId());
  UMA_HISTOGRAM_TIMES("ServiceWorkerCache.CacheStorage.Match",
                      TimeTicks::Now() - start_time);

  WebServiceWorkerCacheStorage::CacheStorageMatchCallbacks* callbacks =
      match_callbacks_.Lookup(request_id);
  if (result->is_status() &&
      result->get_status() != CacheStorageError::kSuccess) {
    callbacks->OnError(result->get_status());
  } else {
    blink::WebServiceWorkerResponse web_response;
    PopulateWebResponseFromResponse(result->get_response(), &web_response);

    callbacks->OnSuccess(web_response);
  }
  match_callbacks_.Remove(request_id);
}

void CacheStorageDispatcher::dispatchHas(
    std::unique_ptr<WebServiceWorkerCacheStorage::CacheStorageCallbacks>
        callbacks,
    const blink::WebString& cacheName,
    service_manager::InterfaceProvider* provider) {
  int request_id = has_callbacks_.Add(std::move(callbacks));
  SetupInterface(provider);
  cache_storage_ptr_->Has(
      cacheName.Utf16(),
      base::BindOnce(&CacheStorageDispatcher::OnCacheStorageHasCallback,
                     base::Unretained(this), CurrentWorkerId(), request_id,
                     base::TimeTicks::Now()));
}

void CacheStorageDispatcher::dispatchOpen(
    std::unique_ptr<
        WebServiceWorkerCacheStorage::CacheStorageWithCacheCallbacks> callbacks,
    const blink::WebString& cacheName,
    service_manager::InterfaceProvider* provider) {
  int request_id = open_callbacks_.Add(std::move(callbacks));
  SetupInterface(provider);
  cache_storage_ptr_->Open(
      cacheName.Utf16(),
      base::BindOnce(&CacheStorageDispatcher::OnCacheStorageOpenCallback,
                     base::Unretained(this), CurrentWorkerId(), request_id,
                     base::TimeTicks::Now()));
}

void CacheStorageDispatcher::dispatchDelete(
    std::unique_ptr<WebServiceWorkerCacheStorage::CacheStorageCallbacks>
        callbacks,
    const blink::WebString& cacheName,
    service_manager::InterfaceProvider* provider) {
  int request_id = delete_callbacks_.Add(std::move(callbacks));

  SetupInterface(provider);
  cache_storage_ptr_->Delete(
      cacheName.Utf16(),
      base::BindOnce(&CacheStorageDispatcher::CacheStorageDeleteCallback,
                     base::Unretained(this), CurrentWorkerId(), request_id,
                     base::TimeTicks::Now()));
}

void CacheStorageDispatcher::SetupInterface(
    service_manager::InterfaceProvider* provider) {
  if (!cache_storage_ptr_) {
    provider->GetInterface(mojo::MakeRequest(&cache_storage_ptr_));
  }
}

void CacheStorageDispatcher::dispatchKeys(
    std::unique_ptr<WebServiceWorkerCacheStorage::CacheStorageKeysCallbacks>
        callbacks,
    service_manager::InterfaceProvider* provider) {
  int request_id = keys_callbacks_.Add(std::move(callbacks));

  SetupInterface(provider);
  cache_storage_ptr_->Keys(base::BindOnce(
      &CacheStorageDispatcher::KeysCallback, base::Unretained(this),
      CurrentWorkerId(), request_id, base::TimeTicks::Now()));
}

void CacheStorageDispatcher::dispatchMatch(
    std::unique_ptr<WebServiceWorkerCacheStorage::CacheStorageMatchCallbacks>
        callbacks,
    const blink::WebServiceWorkerRequest& request,
    const blink::WebServiceWorkerCache::QueryParams& query_params,
    service_manager::InterfaceProvider* provider) {
  int request_id = match_callbacks_.Add(std::move(callbacks));
  SetupInterface(provider);
  cache_storage_ptr_->Match(
      FetchRequestFromWebRequest(request),
      QueryParamsFromWebQueryParams(query_params),
      base::BindOnce(&CacheStorageDispatcher::OnCacheStorageMatchCallback,
                     base::Unretained(this), CurrentWorkerId(), request_id,
                     base::TimeTicks::Now()));
}

void CacheStorageDispatcher::PopulateWebResponseFromResponse(
    const ServiceWorkerResponse& response,
    blink::WebServiceWorkerResponse* web_response) {
  web_response->SetURLList(response.url_list);
  web_response->SetStatus(response.status_code);
  web_response->SetStatusText(WebString::FromASCII(response.status_text));
  web_response->SetResponseType(response.response_type);
  web_response->SetResponseTime(response.response_time);
  web_response->SetCacheStorageCacheName(
      response.is_in_cache_storage
          ? blink::WebString::FromUTF8(response.cache_storage_cache_name)
          : blink::WebString());
  blink::WebVector<blink::WebString> headers(
      response.cors_exposed_header_names.size());
  std::transform(response.cors_exposed_header_names.begin(),
                 response.cors_exposed_header_names.end(), headers.begin(),
                 [](const std::string& s) { return WebString::FromLatin1(s); });
  web_response->SetCorsExposedHeaderNames(headers);

  for (const auto& i : response.headers) {
    web_response->SetHeader(WebString::FromASCII(i.first),
                            WebString::FromASCII(i.second));
  }

  if (!response.blob_uuid.empty()) {
    DCHECK(response.blob);
    mojo::ScopedMessagePipeHandle blob_pipe;
    if (response.blob)
      blob_pipe = response.blob->Clone().PassInterface().PassHandle();
    web_response->SetBlob(blink::WebString::FromUTF8(response.blob_uuid),
                          response.blob_size, std::move(blob_pipe));
    // Let the host know that it can release its reference to the blob.
    cache_storage_ptr_->BlobDataHandled(response.blob_uuid);
  }
}

blink::WebVector<blink::WebServiceWorkerResponse>
CacheStorageDispatcher::WebResponsesFromResponses(
    const std::vector<ServiceWorkerResponse>& responses) {
  blink::WebVector<blink::WebServiceWorkerResponse> web_responses(
      responses.size());
  for (size_t i = 0; i < responses.size(); ++i)
    PopulateWebResponseFromResponse(responses[i], &(web_responses[i]));
  return web_responses;
}

}  // namespace content
