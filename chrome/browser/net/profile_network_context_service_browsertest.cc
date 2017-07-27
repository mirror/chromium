// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/net/profile_network_context_service.h"

#include <string>

#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/threading/thread_restrictions.h"
#include "chrome/browser/net/profile_network_context_service.h"
#include "chrome/browser/net/profile_network_context_service_factory.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/common/chrome_constants.h"
#include "chrome/common/pref_names.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "components/prefs/pref_service.h"
#include "content/public/common/content_features.h"
#include "content/public/common/network_service.mojom.h"
#include "content/public/common/resource_response.h"
#include "content/public/common/resource_response_info.h"
#include "content/public/common/url_loader.mojom.h"
#include "content/public/common/url_loader_factory.mojom.h"
#include "content/public/test/test_url_loader_client.h"
#include "net/traffic_annotation/network_traffic_annotation_test_helper.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace {

// Added to the disk cache path in a test.
const base::FilePath::CharType kBonusCachePath[] = FILE_PATH_LITERAL("foo");

enum class NetworkServiceState {
  kDisabled,
  kEnabled,
};

// Most tests for this class are in NetworkContextConfigurationBrowserTest.
class ProfileNetworkContextServiceBrowsertest
    : public InProcessBrowserTest,
      public testing::WithParamInterface<NetworkServiceState> {
 public:
  ProfileNetworkContextServiceBrowsertest() {
    EXPECT_TRUE(embedded_test_server()->Start());
  }

  ~ProfileNetworkContextServiceBrowsertest() override {}

  void SetUpInProcessBrowserTestFixture() override {
    if (GetParam() == NetworkServiceState::kEnabled)
      feature_list_.InitAndEnableFeature(features::kNetworkService);
  }

  void SetUpOnMainThread() override {
    network_context_ = ProfileNetworkContextServiceFactory::GetInstance()
                           ->GetForContext(browser()->profile())
                           ->MainContext();
    network_context_->CreateURLLoaderFactory(MakeRequest(&loader_factory_), 0);
  }

  content::mojom::URLLoaderFactory* loader_factory() const {
    return loader_factory_.get();
  }

 private:
  base::test::ScopedFeatureList feature_list_;
  content::mojom::NetworkContext* network_context_ = nullptr;
  content::mojom::URLLoaderFactoryPtr loader_factory_;
};

IN_PROC_BROWSER_TEST_P(ProfileNetworkContextServiceBrowsertest,
                       PRE_DiskCacheLocation) {
  // Start a request, to give the network service time to create a cache
  // directory.
  content::mojom::URLLoaderPtr loader;
  content::ResourceRequest request;
  content::TestURLLoaderClient client;
  request.url = embedded_test_server()->GetURL("/cachetime");
  request.method = "GET";
  loader_factory()->CreateLoaderAndStart(
      mojo::MakeRequest(&loader), 2, 1, content::mojom::kURLLoadOptionNone,
      request, client.CreateInterfacePtr(),
      net::MutableNetworkTrafficAnnotationTag(TRAFFIC_ANNOTATION_FOR_TESTS));
  client.RunUntilResponseReceived();
  ASSERT_TRUE(client.response_head().headers);
  EXPECT_EQ(200, client.response_head().headers->response_code());
  client.RunUntilResponseBodyArrived();

  base::ThreadRestrictions::ScopedAllowIO allow_io;
  base::FilePath cache_path =
      browser()->profile()->GetPath().Append(chrome::kCacheDirname);
  EXPECT_TRUE(base::PathExists(cache_path));

  base::FilePath bonus_cache_path = browser()
                                        ->profile()
                                        ->GetPath()
                                        .Append(kBonusCachePath)
                                        .Append(chrome::kCacheDirname);
  EXPECT_FALSE(base::PathExists(bonus_cache_path));

  browser()->profile()->GetPrefs()->SetFilePath(
      prefs::kDiskCacheDir, base::FilePath(kBonusCachePath));
}

IN_PROC_BROWSER_TEST_P(ProfileNetworkContextServiceBrowsertest,
                       DiskCacheLocation) {
  EXPECT_EQ(
      base::FilePath(kBonusCachePath),
      browser()->profile()->GetPrefs()->GetFilePath(prefs::kDiskCacheDir));

  // Start a request, to give the network service time to create a cache
  // directory.
  content::mojom::URLLoaderPtr loader;
  content::ResourceRequest request;
  content::TestURLLoaderClient client;
  request.url = embedded_test_server()->GetURL("/cachetime");
  request.method = "GET";
  loader_factory()->CreateLoaderAndStart(
      mojo::MakeRequest(&loader), 2, 1, content::mojom::kURLLoadOptionNone,
      request, client.CreateInterfacePtr(),
      net::MutableNetworkTrafficAnnotationTag(TRAFFIC_ANNOTATION_FOR_TESTS));
  client.RunUntilResponseReceived();
  ASSERT_TRUE(client.response_head().headers);
  EXPECT_EQ(200, client.response_head().headers->response_code());
  client.RunUntilResponseBodyArrived();

  base::ThreadRestrictions::ScopedAllowIO allow_io;
  base::FilePath bonus_cache_path = browser()
                                        ->profile()
                                        ->GetPath()
                                        .Append(kBonusCachePath)
                                        .Append(chrome::kCacheDirname);
  EXPECT_TRUE(base::PathExists(bonus_cache_path));
}

INSTANTIATE_TEST_CASE_P(
    /* No test prefix */,
    ProfileNetworkContextServiceBrowsertest,
    ::testing::Values(NetworkServiceState::kDisabled,
                      NetworkServiceState::kEnabled));

}  // namespace
