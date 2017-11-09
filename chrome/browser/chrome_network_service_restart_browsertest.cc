// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/test/scoped_feature_list.h"
#include "build/build_config.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/storage_partition.h"
#include "content/public/common/content_features.h"
#include "content/public/common/network_service_test.mojom.h"
#include "content/public/common/service_manager_connection.h"
#include "content/public/common/service_names.mojom.h"
#include "content/public/test/browser_test.h"
#include "content/public/test/browser_test_utils.h"
#include "content/public/test/simple_url_loader_test_helper.h"
#include "net/traffic_annotation/network_traffic_annotation_test_helper.h"
#include "services/service_manager/public/cpp/connector.h"

namespace content {

class ChromeNetworkServiceRestartBrowserTest : public InProcessBrowserTest {
 public:
  ChromeNetworkServiceRestartBrowserTest() {
    scoped_feature_list_.InitAndEnableFeature(features::kNetworkService);
    EXPECT_TRUE(embedded_test_server()->Start());
  }

  void SimulateNetworkServiceCrash() {
    mojom::NetworkServiceTestPtr network_service_test;
    ServiceManagerConnection::GetForProcess()->GetConnector()->BindInterface(
        mojom::kNetworkServiceName, &network_service_test);
    network_service_test->SimulateCrash();
  }

  int LoadBasicRequest(mojom::NetworkContext* network_context) {
    mojom::URLLoaderFactoryPtr url_loader_factory;
    network_context->CreateURLLoaderFactory(MakeRequest(&url_loader_factory),
                                            0);

    content::SimpleURLLoaderTestHelper simple_loader_helper;
    std::unique_ptr<content::SimpleURLLoader> simple_loader =
        content::SimpleURLLoader::Create();

    ResourceRequest request;
    request.url = embedded_test_server()->GetURL("/echo");

    simple_loader->DownloadToStringOfUnboundedSizeUntilCrashAndDie(
        request, url_loader_factory.get(), TRAFFIC_ANNOTATION_FOR_TESTS,
        simple_loader_helper.GetCallback());
    simple_loader_helper.WaitForCallback();

    return simple_loader->NetError();
  }

 private:
  base::test::ScopedFeatureList scoped_feature_list_;

  DISALLOW_COPY_AND_ASSIGN(ChromeNetworkServiceRestartBrowserTest);
};

// Make sure |StoragePartition::GetNetworkContext()| returns valid interface
// after crash.
IN_PROC_BROWSER_TEST_F(ChromeNetworkServiceRestartBrowserTest,
                       StoragePartitionImplGetNetworkContext) {
#if defined(OS_MACOSX)
  // |NetworkServiceTestHelper| doesn't work on browser_tests on macOS.
  return;
#endif
  StoragePartition* partition =
      BrowserContext::GetDefaultStoragePartition(browser()->profile());

  mojom::NetworkContext* network_context = partition->GetNetworkContext();
  EXPECT_EQ(net::OK, LoadBasicRequest(network_context));

  // Crash the NetworkService process. Existing interfaces should no longer
  // work.
  SimulateNetworkServiceCrash();
  EXPECT_EQ(net::ERR_FAILED, LoadBasicRequest(network_context));

  // StoragePartitionImpl should reconnect automatically and return valid
  // interface.
  EXPECT_EQ(net::OK, LoadBasicRequest(partition->GetNetworkContext()));
}

}  // namespace content
