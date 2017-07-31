// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_DEVTOOLS_PROTOCOL_STORAGE_HANDLER_H_
#define CONTENT_BROWSER_DEVTOOLS_PROTOCOL_STORAGE_HANDLER_H_

#include <memory>
#include <string>

#include "base/containers/flat_set.h"
#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "content/browser/cache_storage/cache_storage_context_impl.h"
#include "content/browser/devtools/protocol/devtools_domain_handler.h"
#include "content/browser/devtools/protocol/storage.h"

namespace url {
class Origin;
}

namespace content {

class RenderFrameHostImpl;

namespace protocol {

class StorageHandler : public DevToolsDomainHandler,
                       public Storage::Backend {
 public:
  StorageHandler();
  ~StorageHandler() override;

  void Wire(UberDispatcher* dispatcher) override;
  void SetRenderFrameHost(RenderFrameHostImpl* host) override;

  Response ClearDataForOrigin(
      const std::string& origin,
      const std::string& storage_types) override;
  void GetUsageAndQuota(
      const String& origin,
      std::unique_ptr<GetUsageAndQuotaCallback> callback) override;
  void TrackOrigin(const std::string& origin,
                   std::unique_ptr<TrackOriginCallback> callback) override;
  void UntrackOrigin(const std::string& origin,
                     std::unique_ptr<UntrackOriginCallback> callback) override;

  void NotifyCacheStorageListChanged(const std::string& origin);
  void NotifyCacheStorageContentChanged(const std::string& origin,
                                        const std::string& name);

 private:
  // Observer that listens on the IO thread for cache storage notifications
  // and informs the StorageHandler on the UI for origins of interest.
  // Created on the UI thread but predominantly used and deleted on the IO
  // thread.
  // Registered on creation as an observer in CacheStorageContext, unregistered
  // on destruction
  class CacheStorageObserver : CacheStorageContextImpl::Observer {
   public:
    CacheStorageObserver(base::WeakPtr<StorageHandler> owning_storage_handler,
                         CacheStorageContextImpl* cache_storage_context);
    ~CacheStorageObserver() override;

    // We only care about some origins, maintain the collection on
    // the IO thread to avoid mutex contention.
    void TrackOrigin(const url::Origin& origin);
    void UntrackOrigin(const url::Origin& origin);

   private:
    void AddObserverOnIOThread();
    void TrackOriginOnIOThread(const url::Origin& origin);
    void UntrackOriginOnIOThread(const url::Origin& origin);

    // Plumb observer notifications to the UI thread.
    void OnCacheListChanged(const url::Origin& origin) override;
    void OnCacheContentChanged(const url::Origin& origin,
                               const std::string& cache_name) override;

    base::flat_set<url::Origin> origins_;
    base::WeakPtr<StorageHandler> owner_;
    scoped_refptr<CacheStorageContextImpl> context_;

    DISALLOW_COPY_AND_ASSIGN(CacheStorageObserver);
  };

  CacheStorageObserver* GetCacheStorageObserver();

  std::unique_ptr<Storage::Frontend> frontend_;
  RenderFrameHostImpl* host_;
  std::unique_ptr<CacheStorageObserver> cache_storage_observer_;

  base::WeakPtrFactory<StorageHandler> ptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(StorageHandler);
};

}  // namespace protocol
}  // namespace content

#endif  // CONTENT_BROWSER_DEVTOOLS_PROTOCOL_STORAGE_HANDLER_H_
