// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/autocomplete/chrome_autocomplete_provider_client.h"

#include "base/callback_forward.h"
#include "base/memory/ptr_util.h"
#include "build/build_config.h"
#include "chrome/test/base/testing_profile.h"
#include "content/public/browser/service_worker_context.h"
#include "content/public/browser/storage_partition.h"
#include "content/public/test/test_browser_thread_bundle.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"

// Fixme: put this in a public place? If so, share with
// content/browser/browsing_data/browsing_data_remover_impl_unittest.cc.
/*
class TestStoragePartition : public content::StoragePartition {
 public:
  TestStoragePartition() {}
  ~TestStoragePartition() override {}

  ServiceWorkerContext* GetServiceWorkerContext() override { return nullptr; }

 private:
  DISALLOW_COPY_AND_ASSIGN(TestStoragePartition);
};
*/

namespace content {
class AppCacheService;
class DOMStorageContext;
class HostZoomLevelContext;
class HostZoomMap;
class IndexedDBContext;
class PlatformNotificationContext;
class ServiceWorkerContext;
class ServiceWorkerContextObserver;
class ZoomLevelDelegate;
}  // namespace content

namespace mojom {
class NetworkContext;
}

using ::testing::_;

class MockServiceWorkerContext : public content::ServiceWorkerContext {
 public:
  MockServiceWorkerContext() = default;
  ~MockServiceWorkerContext() override = default;

  MOCK_METHOD1(AddObserver, void(content::ServiceWorkerContextObserver*));
  MOCK_METHOD1(RemoveObserver, void(content::ServiceWorkerContextObserver*));
  MOCK_METHOD3(RegisterServiceWorker,
               void(const content::ServiceWorkerContext::Scope&,
                    const GURL&,
                    const content::ServiceWorkerContext::ResultCallback&));
  MOCK_METHOD2(StartingExternalRequest, bool(int64_t, const std::string&));
  MOCK_METHOD2(FinishedExternalRequest, bool(int64_t, const std::string&));
  // StartActiveWorkerForPattern cannot be mocked because OnceClosure is not
  // copyable.
  void StartActiveWorkerForPattern(
      const GURL&,
      content::ServiceWorkerContext::StartActiveWorkerCallback,
      base::OnceClosure) {}
  MOCK_METHOD2(UnregisterServiceWorker,
               void(const content::ServiceWorkerContext::Scope&,
                    const content::ServiceWorkerContext::ResultCallback&));
  MOCK_METHOD1(
      GetAllOriginsInfo,
      void(const content::ServiceWorkerContext::GetUsageInfoCallback&));
  MOCK_METHOD2(DeleteForOrigin,
               void(const GURL&,
                    const content::ServiceWorkerContext::ResultCallback&));
  MOCK_METHOD3(
      CheckHasServiceWorker,
      void(
          const GURL&,
          const GURL&,
          const content::ServiceWorkerContext::CheckHasServiceWorkerCallback&));
  MOCK_METHOD2(
      CountExternalRequestsForTest,
      void(
          const GURL&,
          const content::ServiceWorkerContext::CountExternalRequestsCallback&));
  MOCK_METHOD1(StopAllServiceWorkersForOrigin, void(const GURL&));
  MOCK_METHOD1(ClearAllServiceWorkersForTest, void(const base::Closure&));
  MOCK_METHOD2(StartServiceWorkerForNavigationHint,
               void(const GURL&,
                    const StartServiceWorkerForNavigationHintCallback&));
};

class TestStoragePartition : public content::StoragePartition {
 public:
  // FIXME: = default here?
  TestStoragePartition() {}
  ~TestStoragePartition() override {}

  // StoragePartition implementation.
  base::FilePath GetPath() override { return base::FilePath(); }
  net::URLRequestContextGetter* GetURLRequestContext() override {
    return nullptr;
  }
  net::URLRequestContextGetter* GetMediaURLRequestContext() override {
    return nullptr;
  }
  storage::QuotaManager* GetQuotaManager() override { return nullptr; }
  content::AppCacheService* GetAppCacheService() override { return nullptr; }
  storage::FileSystemContext* GetFileSystemContext() override {
    return nullptr;
  }
  storage::DatabaseTracker* GetDatabaseTracker() override { return nullptr; }
  content::DOMStorageContext* GetDOMStorageContext() override {
    return nullptr;
  }
  content::IndexedDBContext* GetIndexedDBContext() override { return nullptr; }
  content::ServiceWorkerContext* GetServiceWorkerContext() override {
    return nullptr;
  }
  content::CacheStorageContext* GetCacheStorageContext() override {
    return nullptr;
  }
  content::PlatformNotificationContext* GetPlatformNotificationContext()
      override {
    return nullptr;
  }
#if !defined(OS_ANDROID)
  content::HostZoomMap* GetHostZoomMap() override { return nullptr; }
  content::HostZoomLevelContext* GetHostZoomLevelContext() override {
    return nullptr;
  }
  content::ZoomLevelDelegate* GetZoomLevelDelegate() override {
    return nullptr;
  }
#endif  // !defined(OS_ANDROID)

  void ClearDataForOrigin(uint32_t remove_mask,
                          uint32_t quota_storage_remove_mask,
                          const GURL& storage_origin,
                          net::URLRequestContextGetter* rq_context,
                          const base::Closure& callback) override {}

  void ClearData(uint32_t remove_mask,
                 uint32_t quota_storage_remove_mask,
                 const GURL& storage_origin,
                 const OriginMatcherFunction& origin_matcher,
                 const base::Time begin,
                 const base::Time end,
                 const base::Closure& callback) override {}

  void ClearData(uint32_t remove_mask,
                 uint32_t quota_storage_remove_mask,
                 const OriginMatcherFunction& origin_matcher,
                 const CookieMatcherFunction& cookie_matcher,
                 const base::Time begin,
                 const base::Time end,
                 const base::Closure& callback) override {}

  void ClearHttpAndMediaCaches(
      const base::Time begin,
      const base::Time end,
      const base::Callback<bool(const GURL&)>& url_matcher,
      const base::Closure& callback) override {}

  void Flush() override {}

  void ClearBluetoothAllowedDevicesMapForTesting() override {}

 private:
  DISALLOW_COPY_AND_ASSIGN(TestStoragePartition);
};

class ProviderClientTestStoragePartition : public TestStoragePartition {
 public:
  ProviderClientTestStoragePartition() {}
  ~ProviderClientTestStoragePartition() override {}

  void set_service_worker_context(content::ServiceWorkerContext* context) {
    service_worker_context_ = context;
  }
  content::ServiceWorkerContext* GetServiceWorkerContext() override {
    return service_worker_context_;
  }

 private:
  content::ServiceWorkerContext* service_worker_context_;

  DISALLOW_COPY_AND_ASSIGN(ProviderClientTestStoragePartition);
};

class ChromeAutocompleteProviderClientTest : public testing::Test {
 public:
  ChromeAutocompleteProviderClientTest() {}
  ~ChromeAutocompleteProviderClientTest() override {}

  void SetUp() override {
    client_ = base::MakeUnique<ChromeAutocompleteProviderClient>(&profile_);
    storage_partition_.set_service_worker_context(&service_worker_context_);
    client_->set_storage_partition(&storage_partition_);
  }

  void GoOffTheRecord() {
    client_ = base::MakeUnique<ChromeAutocompleteProviderClient>(
        profile_.GetOffTheRecordProfile());
  }

 protected:
  content::TestBrowserThreadBundle test_browser_thread_bundle_;
  TestingProfile profile_;
  std::unique_ptr<ChromeAutocompleteProviderClient> client_;
  ProviderClientTestStoragePartition storage_partition_;
  MockServiceWorkerContext service_worker_context_;

 private:
  DISALLOW_COPY_AND_ASSIGN(ChromeAutocompleteProviderClientTest);
};

TEST_F(ChromeAutocompleteProviderClientTest, StartServiceWorker) {
  GURL destination_url("https://google.com/search?q=puppies");
  EXPECT_CALL(service_worker_context_,
              StartServiceWorkerForNavigationHint(destination_url, _));
  client_->StartServiceWorker(destination_url);
}

TEST_F(ChromeAutocompleteProviderClientTest,
       DontStartServiceWorkerInIncognito) {
  GoOffTheRecord();
  GURL destination_url("https://google.com/search?q=puppies");
  EXPECT_CALL(service_worker_context_,
              StartServiceWorkerForNavigationHint(destination_url, _))
      .Times(0);
  client_->StartServiceWorker(destination_url);
}
