// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "webkit/appcache/appcache_service.h"

#include "base/logging.h"
#include "base/message_loop.h"
#include "base/stl_util-inl.h"
#include "webkit/appcache/appcache_backend_impl.h"
#include "webkit/appcache/appcache_entry.h"
#include "webkit/appcache/appcache_quota_client.h"
#include "webkit/appcache/appcache_storage_impl.h"
#include "webkit/quota/quota_manager.h"
#include "webkit/quota/special_storage_policy.h"

namespace appcache {

AppCacheInfoCollection::AppCacheInfoCollection() {}

AppCacheInfoCollection::~AppCacheInfoCollection() {}

// AsyncHelper -------

class AppCacheService::AsyncHelper
    : public AppCacheStorage::Delegate {
 public:
  AsyncHelper(
      AppCacheService* service, net::CompletionCallback* callback)
      : service_(service), callback_(callback) {
    service_->pending_helpers_.insert(this);
  }

  virtual ~AsyncHelper() {
    if (service_)
      service_->pending_helpers_.erase(this);
  }

  virtual void Start() = 0;
  virtual void Cancel();

 protected:
  void CallCallback(int rv) {
    if (callback_) {
      // Defer to guarentee async completion.
      MessageLoop::current()->PostTask(
          FROM_HERE,
          NewRunnableFunction(&DeferredCallCallback, callback_, rv));
    }
    callback_ = NULL;
  }

  static void DeferredCallCallback(net::CompletionCallback* callback, int rv) {
    callback->Run(rv);
  }

  AppCacheService* service_;
  net::CompletionCallback* callback_;
};

void AppCacheService::AsyncHelper::Cancel() {
  if (callback_) {
    callback_->Run(net::ERR_ABORTED);
    callback_ = NULL;
  }
  service_->storage()->CancelDelegateCallbacks(this);
  service_ = NULL;
}

// CanHandleOfflineHelper -------

class AppCacheService::CanHandleOfflineHelper : AsyncHelper {
 public:
  CanHandleOfflineHelper(
      AppCacheService* service, const GURL& url,
      net::CompletionCallback* callback)
      : AsyncHelper(service, callback), url_(url) {
  }

  virtual void Start() {
    service_->storage()->FindResponseForMainRequest(url_, GURL(), this);
  }

 private:
  // AppCacheStorage::Delegate override
  virtual void OnMainResponseFound(
      const GURL& url, const AppCacheEntry& entry,
      const GURL& fallback_url, const AppCacheEntry& fallback_entry,
      int64 cache_id, const GURL& mainfest_url,
      bool was_blocked_by_policy);

  GURL url_;
  DISALLOW_COPY_AND_ASSIGN(CanHandleOfflineHelper);
};

void AppCacheService::CanHandleOfflineHelper::OnMainResponseFound(
      const GURL& url, const AppCacheEntry& entry,
      const GURL& fallback_url, const AppCacheEntry& fallback_entry,
      int64 cache_id, const GURL& mainfest_url,
      bool was_blocked_by_policy) {
  bool can = !was_blocked_by_policy &&
             (entry.has_response_id() || fallback_entry.has_response_id());
  CallCallback(can ? net::OK : net::ERR_FAILED);
  delete this;
}

// DeleteHelper -------

class AppCacheService::DeleteHelper : public AsyncHelper {
 public:
  DeleteHelper(
      AppCacheService* service, const GURL& manifest_url,
      net::CompletionCallback* callback)
      : AsyncHelper(service, callback), manifest_url_(manifest_url) {
  }

  virtual void Start() {
    service_->storage()->LoadOrCreateGroup(manifest_url_, this);
  }

 private:
  // AppCacheStorage::Delegate methods
  virtual void OnGroupLoaded(
      appcache::AppCacheGroup* group, const GURL& manifest_url);
  virtual void OnGroupMadeObsolete(
      appcache::AppCacheGroup* group, bool success);

  GURL manifest_url_;
  DISALLOW_COPY_AND_ASSIGN(DeleteHelper);
};

void AppCacheService::DeleteHelper::OnGroupLoaded(
      appcache::AppCacheGroup* group, const GURL& manifest_url) {
  if (group) {
    group->set_being_deleted(true);
    group->CancelUpdate();
    service_->storage()->MakeGroupObsolete(group, this);
  } else {
    CallCallback(net::ERR_FAILED);
    delete this;
  }
}

void AppCacheService::DeleteHelper::OnGroupMadeObsolete(
      appcache::AppCacheGroup* group, bool success) {
  CallCallback(success ? net::OK : net::ERR_FAILED);
  delete this;
}

// DeleteOriginHelper -------

class AppCacheService::DeleteOriginHelper : public AsyncHelper {
 public:
  DeleteOriginHelper(
      AppCacheService* service, const GURL& origin,
      net::CompletionCallback* callback)
      : AsyncHelper(service, callback), origin_(origin),
        num_caches_to_delete_(0), successes_(0), failures_(0) {
  }

  virtual void Start() {
    // We start by listing all caches, continues in OnAllInfo().
    service_->storage()->GetAllInfo(this);
  }

 private:
  // AppCacheStorage::Delegate methods
  virtual void OnAllInfo(AppCacheInfoCollection* collection);
  virtual void OnGroupLoaded(
      appcache::AppCacheGroup* group, const GURL& manifest_url);
  virtual void OnGroupMadeObsolete(
      appcache::AppCacheGroup* group, bool success);

  void CacheCompleted(bool success);

  GURL origin_;
  int num_caches_to_delete_;
  int successes_;
  int failures_;
  DISALLOW_COPY_AND_ASSIGN(DeleteOriginHelper);
};

void AppCacheService::DeleteOriginHelper::OnAllInfo(
      AppCacheInfoCollection* collection) {
  if (!collection) {
    // Failed to get a listing.
    CallCallback(net::ERR_FAILED);
    delete this;
    return;
  }
  std::map<GURL, AppCacheInfoVector>::iterator found =
      collection->infos_by_origin.find(origin_);
  if (found == collection->infos_by_origin.end() || found->second.empty()) {
    // No caches for this origin.
    CallCallback(net::OK);
    delete this;
    return;
  }

  // We have some caches to delete.
  const AppCacheInfoVector& caches_to_delete = found->second;
  successes_ = 0;
  failures_ = 0;
  num_caches_to_delete_ = static_cast<int>(caches_to_delete.size());
  for (AppCacheInfoVector::const_iterator iter = caches_to_delete.begin();
       iter != caches_to_delete.end(); ++iter) {
    service_->storage()->LoadOrCreateGroup(iter->manifest_url, this);
  }
}

void AppCacheService::DeleteOriginHelper::OnGroupLoaded(
      appcache::AppCacheGroup* group, const GURL& manifest_url) {
  if (group) {
    group->set_being_deleted(true);
    group->CancelUpdate();
    service_->storage()->MakeGroupObsolete(group, this);
  } else {
    CacheCompleted(false);
  }
}

void AppCacheService::DeleteOriginHelper::OnGroupMadeObsolete(
      appcache::AppCacheGroup* group, bool success) {
  CacheCompleted(success);
}

void AppCacheService::DeleteOriginHelper::CacheCompleted(bool success) {
  if (success)
    ++successes_;
  else
    ++failures_;
  if ((successes_ + failures_) < num_caches_to_delete_)
    return;

  CallCallback(!failures_ ? net::OK : net::ERR_FAILED);
  delete this;
}


// GetInfoHelper -------

class AppCacheService::GetInfoHelper : AsyncHelper {
 public:
  GetInfoHelper(
      AppCacheService* service, AppCacheInfoCollection* collection,
      net::CompletionCallback* callback)
      : AsyncHelper(service, callback), collection_(collection) {
  }

  virtual void Start() {
    service_->storage()->GetAllInfo(this);
  }

 private:
  // AppCacheStorage::Delegate override
  virtual void OnAllInfo(AppCacheInfoCollection* collection);

  scoped_refptr<AppCacheInfoCollection> collection_;
  DISALLOW_COPY_AND_ASSIGN(GetInfoHelper);
};

void AppCacheService::GetInfoHelper::OnAllInfo(
      AppCacheInfoCollection* collection) {
  if (collection)
    collection->infos_by_origin.swap(collection_->infos_by_origin);
  CallCallback(collection ? net::OK : net::ERR_FAILED);
  delete this;
}


// AppCacheService -------

AppCacheService::AppCacheService(quota::QuotaManagerProxy* quota_manager_proxy)
    : appcache_policy_(NULL), quota_client_(NULL),
      quota_manager_proxy_(quota_manager_proxy),
      request_context_(NULL)  {
  if (quota_manager_proxy_) {
    quota_client_ = new AppCacheQuotaClient(this);
    quota_manager_proxy_->RegisterClient(quota_client_);
  }
}

AppCacheService::~AppCacheService() {
  DCHECK(backends_.empty());
  std::for_each(pending_helpers_.begin(),
                pending_helpers_.end(),
                std::mem_fun(&AsyncHelper::Cancel));
  STLDeleteElements(&pending_helpers_);
  if (quota_client_)
    quota_client_->NotifyAppCacheDestroyed();
}

void AppCacheService::Initialize(const FilePath& cache_directory,
                                 base::MessageLoopProxy* cache_thread) {
  DCHECK(!storage_.get());
  AppCacheStorageImpl* storage = new AppCacheStorageImpl(this);
  storage->Initialize(cache_directory, cache_thread);
  storage_.reset(storage);
}

void AppCacheService::CanHandleMainResourceOffline(
    const GURL& url,
    net::CompletionCallback* callback) {
  CanHandleOfflineHelper* helper =
      new CanHandleOfflineHelper(this, url, callback);
  helper->Start();
}

void AppCacheService::GetAllAppCacheInfo(AppCacheInfoCollection* collection,
                                         net::CompletionCallback* callback) {
  DCHECK(collection);
  GetInfoHelper* helper = new GetInfoHelper(this, collection, callback);
  helper->Start();
}

void AppCacheService::DeleteAppCacheGroup(const GURL& manifest_url,
                                          net::CompletionCallback* callback) {
  DeleteHelper* helper = new DeleteHelper(this, manifest_url, callback);
  helper->Start();
}

void AppCacheService::DeleteAppCachesForOrigin(
    const GURL& origin,  net::CompletionCallback* callback) {
  DeleteOriginHelper* helper = new DeleteOriginHelper(this, origin, callback);
  helper->Start();
}

void AppCacheService::set_special_storage_policy(
    quota::SpecialStoragePolicy* policy) {
  special_storage_policy_ = policy;
}

void AppCacheService::RegisterBackend(
    AppCacheBackendImpl* backend_impl) {
  DCHECK(backends_.find(backend_impl->process_id()) == backends_.end());
  backends_.insert(
      BackendMap::value_type(backend_impl->process_id(), backend_impl));
}

void AppCacheService::UnregisterBackend(
    AppCacheBackendImpl* backend_impl) {
  backends_.erase(backend_impl->process_id());
}

}  // namespace appcache
