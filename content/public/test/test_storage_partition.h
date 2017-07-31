// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_PUBLIC_TEST_TEST_STORAGE_PARTITION_H_
#define CONTENT_PUBLIC_TEST_TEST_STORAGE_PARTITION_H_

#include "base/callback_forward.h"
#include "base/files/file_path.h"
#include "build/build_config.h"
#include "content/public/browser/storage_partition.h"

namespace net {
class URLRequestContextGetter;
}

namespace content {

class AppCacheService;
class DOMStorageContext;
class IndexedDBContext;
class PlatformNotificationContext;
class ServiceWorkerContext;

#if !defined(OS_ANDROID)
class HostZoomLevelContext;
class HostZoomMap;
class ZoomLevelDelegate;
#endif  // !defined(OS_ANDROID)

namespace mojom {
class NetworkContext;
}

// Fake implementation of StoragePartition.
class TestStoragePartition : public StoragePartition {
 public:
  TestStoragePartition();
  virtual ~TestStoragePartition();

  void set_path(base::FilePath file_path) { file_path_ = file_path; }
  base::FilePath GetPath() override { return file_path_; }

  void set_url_request_context(net::URLRequestContextGetter* getter) {
    url_request_context_getter_ = getter;
  }
  net::URLRequestContextGetter* GetURLRequestContext() override {
    return url_request_context_getter_;
  }

  void set_media_url_request_context(net::URLRequestContextGetter* getter) {
    media_url_request_context_getter_ = getter;
  }
  net::URLRequestContextGetter* GetMediaURLRequestContext() override {
    return media_url_request_context_getter_;
  }

  void set_network_context(mojom::NetworkContext* context) {
    network_context_ = context;
  }
  mojom::NetworkContext* GetNetworkContext() override {
    return network_context_;
  }

  void set_quota_manager(storage::QuotaManager* manager) {
    quota_manager_ = manager;
  }
  storage::QuotaManager* GetQuotaManager() override { return quota_manager_; }

  void set_app_cache_service(AppCacheService* service) {
    app_cache_service_ = service;
  }
  AppCacheService* GetAppCacheService() override { return app_cache_service_; }

  void set_file_system_context(storage::FileSystemContext* context) {
    file_system_context_ = context;
  }
  storage::FileSystemContext* GetFileSystemContext() override {
    return file_system_context_;
  }

  void set_database_tracker(storage::DatabaseTracker* tracker) {
    database_tracker_ = tracker;
  }
  storage::DatabaseTracker* GetDatabaseTracker() override {
    return database_tracker_;
  }

  void set_dom_storage_context(DOMStorageContext* context) {
    dom_storage_context_ = context;
  }
  DOMStorageContext* GetDOMStorageContext() override {
    return dom_storage_context_;
  }

  void set_indexed_db_context(IndexedDBContext* context) {
    indexed_db_context_ = context;
  }
  IndexedDBContext* GetIndexedDBContext() override {
    return indexed_db_context_;
  }

  void set_service_worker_context(ServiceWorkerContext* context) {
    service_worker_context_ = context;
  }
  ServiceWorkerContext* GetServiceWorkerContext() override {
    return service_worker_context_;
  }

  void set_cache_storage_context(CacheStorageContext* context) {
    cache_storage_context_ = context;
  }
  CacheStorageContext* GetCacheStorageContext() override {
    return cache_storage_context_;
  }

  void set_platform_notification_context(PlatformNotificationContext* context) {
    platform_notification_context_ = context;
  }
  PlatformNotificationContext* GetPlatformNotificationContext() override {
    return nullptr;
  }

#if !defined(OS_ANDROID)
  void set_host_zoom_map(HostZoomMap* map) { host_zoom_map_ = map; }
  HostZoomMap* GetHostZoomMap() override { return host_zoom_map_; }

  void set_host_zoom_level_context(HostZoomLevelContext* context) {
    host_zoom_level_context_ = context;
  }
  HostZoomLevelContext* GetHostZoomLevelContext() override {
    return host_zoom_level_context_;
  }

  void set_zoom_level_delegate(ZoomLevelDelegate* delegate) {
    zoom_level_delegate_ = delegate;
  }
  ZoomLevelDelegate* GetZoomLevelDelegate() override {
    return zoom_level_delegate_;
  }
#endif  // !defined(OS_ANDROID)

  void ClearDataForOrigin(uint32_t remove_mask,
                          uint32_t quota_storage_remove_mask,
                          const GURL& storage_origin,
                          net::URLRequestContextGetter* rq_context,
                          const base::Closure& callback) override;

  void ClearData(uint32_t remove_mask,
                 uint32_t quota_storage_remove_mask,
                 const GURL& storage_origin,
                 const OriginMatcherFunction& origin_matcher,
                 const base::Time begin,
                 const base::Time end,
                 const base::Closure& callback) override;

  void ClearData(uint32_t remove_mask,
                 uint32_t quota_storage_remove_mask,
                 const OriginMatcherFunction& origin_matcher,
                 const CookieMatcherFunction& cookie_matcher,
                 const base::Time begin,
                 const base::Time end,
                 const base::Closure& callback) override;

  void ClearHttpAndMediaCaches(
      const base::Time begin,
      const base::Time end,
      const base::Callback<bool(const GURL&)>& url_matcher,
      const base::Closure& callback) override;

  void Flush() override;

  void ClearBluetoothAllowedDevicesMapForTesting() override;

 private:
  base::FilePath file_path_;
  net::URLRequestContextGetter* url_request_context_getter_;
  net::URLRequestContextGetter* media_url_request_context_getter_;
  mojom::NetworkContext* network_context_;
  storage::QuotaManager* quota_manager_;
  AppCacheService* app_cache_service_;
  storage::FileSystemContext* file_system_context_;
  storage::DatabaseTracker* database_tracker_;
  DOMStorageContext* dom_storage_context_;
  IndexedDBContext* indexed_db_context_;
  ServiceWorkerContext* service_worker_context_;
  CacheStorageContext* cache_storage_context_;
  PlatformNotificationContext* platform_notification_context_;
#if !defined(OS_ANDROID)
  HostZoomMap* host_zoom_map_;
  HostZoomLevelContext* host_zoom_level_context_;
  ZoomLevelDelegate* zoom_level_delegate_;
#endif  // !defined(OS_ANDROID)

  DISALLOW_COPY_AND_ASSIGN(TestStoragePartition);
};

}  // namespace content

#endif  // CONTENT_PUBLIC_TEST_TEST_STORAGE_PARTITION_H_
