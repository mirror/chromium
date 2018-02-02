// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_RENDERER_CACHE_STORAGE_CACHE_STORAGE_DISPATCHER_H_
#define CONTENT_RENDERER_CACHE_STORAGE_CACHE_STORAGE_DISPATCHER_H_

#include <stdint.h>

#include <memory>
#include <vector>

#include "base/containers/id_map.h"
#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "base/strings/string16.h"
#include "base/time/time.h"
#include "content/public/renderer/worker_thread.h"
#include "ipc/ipc_message.h"
#include "services/service_manager/public/cpp/interface_provider.h"
#include "third_party/WebKit/public/platform/modules/cache_storage/cache_storage.mojom.h"
#include "third_party/WebKit/public/platform/modules/serviceworker/WebServiceWorkerCache.h"
#include "third_party/WebKit/public/platform/modules/serviceworker/WebServiceWorkerCacheStorage.h"

namespace content {

class ThreadSafeSender;
struct ServiceWorkerResponse;

// Handle the Cache Storage messaging for this context thread. The
// main thread and each worker thread have their own instances.
class CacheStorageDispatcher : public WorkerThread::Observer {
 public:
  explicit CacheStorageDispatcher(ThreadSafeSender* thread_safe_sender);
  ~CacheStorageDispatcher() override;

  // |thread_safe_sender| needs to be passed in because if the call leads to
  // construction it will be needed.
  static CacheStorageDispatcher* ThreadSpecificInstance(
      ThreadSafeSender* thread_safe_sender);

  // WorkerThread::Observer implementation.
  void WillStopCurrentWorkerThread() override;

  // Message handlers for CacheStorage messages from the browser process.
  // And callbacks called by Mojo implementation on Browser process.
  void OnCacheStorageHasCallback(int thread_id,
                                 int request_id,
                                 base::TimeTicks start_time,
                                 blink::mojom::CacheStorageError result);
  void OnCacheStorageOpenCallback(int thread_id,
                                  int request_id,
                                  base::TimeTicks start_time,
                                  blink::mojom::OpenResultPtr result);

  void CacheStorageDeleteCallback(int thread_id,
                                  int request_id,
                                  base::TimeTicks start_time,
                                  blink::mojom::CacheStorageError result);
  void KeysCallback(int thread_id,
                    int request_id,
                    base::TimeTicks start_time,
                    const std::vector<base::string16>& keys);
  void OnCacheStorageMatchCallback(int thread_id,
                                   int request_id,
                                   base::TimeTicks start_time,
                                   blink::mojom::MatchResultPtr result);

  // TODO(jsbell): These are only called by WebServiceWorkerCacheStorageImpl
  // and should be renamed to match Chromium conventions. crbug.com/439389
  void dispatchHas(
      std::unique_ptr<
          blink::WebServiceWorkerCacheStorage::CacheStorageCallbacks> callbacks,
      const blink::WebString& cacheName,
      service_manager::InterfaceProvider* provider);
  void dispatchOpen(
      std::unique_ptr<
          blink::WebServiceWorkerCacheStorage::CacheStorageWithCacheCallbacks>
          callbacks,
      const blink::WebString& cacheName,
      service_manager::InterfaceProvider* provider);
  void dispatchDelete(
      std::unique_ptr<
          blink::WebServiceWorkerCacheStorage::CacheStorageCallbacks> callbacks,
      const blink::WebString& cacheName,
      service_manager::InterfaceProvider* provider);
  void dispatchKeys(
      std::unique_ptr<
          blink::WebServiceWorkerCacheStorage::CacheStorageKeysCallbacks>
          callbacks,
      service_manager::InterfaceProvider* provider);
  void dispatchMatch(
      std::unique_ptr<
          blink::WebServiceWorkerCacheStorage::CacheStorageMatchCallbacks>
          callbacks,
      const blink::WebServiceWorkerRequest& request,
      const blink::WebServiceWorkerCache::QueryParams& query_params,
      service_manager::InterfaceProvider* provider);

 private:
  class WebCache;

  using CallbacksMap = base::IDMap<std::unique_ptr<
      blink::WebServiceWorkerCacheStorage::CacheStorageCallbacks>>;
  using WithCacheCallbacksMap = base::IDMap<std::unique_ptr<
      blink::WebServiceWorkerCacheStorage::CacheStorageWithCacheCallbacks>>;
  using KeysCallbacksMap = base::IDMap<std::unique_ptr<
      blink::WebServiceWorkerCacheStorage::CacheStorageKeysCallbacks>>;
  using StorageMatchCallbacksMap = base::IDMap<std::unique_ptr<
      blink::WebServiceWorkerCacheStorage::CacheStorageMatchCallbacks>>;

  using TimeMap = base::hash_map<int32_t, base::TimeTicks>;

  using MatchCallbacksMap = base::IDMap<
      std::unique_ptr<blink::WebServiceWorkerCache::CacheMatchCallbacks>>;
  using WithResponsesCallbacksMap = base::IDMap<std::unique_ptr<
      blink::WebServiceWorkerCache::CacheWithResponsesCallbacks>>;
  using WithRequestsCallbacksMap = base::IDMap<std::unique_ptr<
      blink::WebServiceWorkerCache::CacheWithRequestsCallbacks>>;
  using BatchCallbacksMap = base::IDMap<
      std::unique_ptr<blink::WebServiceWorkerCache::CacheBatchCallbacks>>;

  static int32_t CurrentWorkerId() { return WorkerThread::GetCurrentId(); }

  void PopulateWebResponseFromResponse(
      const ServiceWorkerResponse& response,
      blink::WebServiceWorkerResponse* web_response);

  blink::WebVector<blink::WebServiceWorkerResponse> WebResponsesFromResponses(
      const std::vector<ServiceWorkerResponse>& responses);

  // Sets up the Mojo InterfacePtr to send IPCs to browswer process.
  void SetupInterface(service_manager::InterfaceProvider* provider);

  scoped_refptr<ThreadSafeSender> thread_safe_sender_;

  // Callbacks for Global Cache Storage methods.
  CallbacksMap has_callbacks_;
  WithCacheCallbacksMap open_callbacks_;
  CallbacksMap delete_callbacks_;
  KeysCallbacksMap keys_callbacks_;
  StorageMatchCallbacksMap match_callbacks_;

  blink::mojom::CacheStoragePtr cache_storage_ptr_;

  base::WeakPtrFactory<CacheStorageDispatcher> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(CacheStorageDispatcher);
};

}  // namespace content

#endif  // CONTENT_RENDERER_CACHE_STORAGE_CACHE_STORAGE_DISPATCHER_H_
