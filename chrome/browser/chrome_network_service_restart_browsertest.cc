// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/test/scoped_feature_list.h"
#include "build/build_config.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/net/system_network_context_manager.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/storage_partition.h"
#include "content/public/common/content_features.h"
#include "content/public/common/content_switches.h"
#include "content/public/common/network_service_test.mojom.h"
#include "content/public/common/service_manager_connection.h"
#include "content/public/common/service_names.mojom.h"
#include "content/public/test/browser_test.h"
#include "content/public/test/browser_test_utils.h"
#include "content/public/test/simple_url_loader_test_helper.h"
#include "content/public/test/test_launcher.h"
#include "net/traffic_annotation/network_traffic_annotation_test_helper.h"
#include "services/service_manager/public/cpp/connector.h"

namespace content {

class ChromeNetworkServiceRestartBrowserTest : public InProcessBrowserTest {
 public:
  ChromeNetworkServiceRestartBrowserTest() {
    scoped_feature_list_.InitAndEnableFeature(features::kNetworkService);
    EXPECT_TRUE(embedded_test_server()->Start());
  }

  // Simulates a network connection change.
  void SimulateNetworkChange(mojom::ConnectionType type) {
    LOG(ERROR) << "Begin ChromeNetworkServiceRestartBrowserTest.SimulateNetworkChange";
    mojom::NetworkServiceTestPtr network_service_test;
    ServiceManagerConnection::GetForProcess()->GetConnector()->BindInterface(
        mojom::kNetworkServiceName, &network_service_test);
    base::RunLoop run_loop;
    LOG(ERROR) << "1. ChromeNetworkServiceRestartBrowserTest.SimulateNetworkChange";
    network_service_test->SimulateNetworkChange(
        type, base::Bind([](base::RunLoop* run_loop) { run_loop->Quit(); },
                         base::Unretained(&run_loop)));
    LOG(ERROR) << "2. ChromeNetworkServiceRestartBrowserTest.SimulateNetworkChange";
    run_loop.Run();
    LOG(ERROR) << "Finished ChromeNetworkServiceRestartBrowserTest.SimulateNetworkChange";
    return;
  }

  void SimulateNetworkServiceCrash() {
    LOG(ERROR) << "Begin ChromeNetworkServiceRestartBrowserTest.SimulateNetworkServiceCrash";
    mojom::NetworkServiceTestPtr network_service_test;
    ServiceManagerConnection::GetForProcess()->GetConnector()->BindInterface(
        mojom::kNetworkServiceName, &network_service_test);
    LOG(ERROR) << "1. ChromeNetworkServiceRestartBrowserTest.SimulateNetworkServiceCrash";
    network_service_test->SimulateCrash();
    LOG(ERROR) << "2. ChromeNetworkServiceRestartBrowserTest.SimulateNetworkServiceCrash";
    network_service_test.FlushForTesting();
    LOG(ERROR) << "Finished ChromeNetworkServiceRestartBrowserTest.SimulateNetworkServiceCrash";
  }

  int LoadBasicRequest(mojom::NetworkContext* network_context) {
    LOG(ERROR) << "Begin ChromeNetworkServiceRestartBrowserTest.LoadBasicRequest";
    mojom::URLLoaderFactoryPtr url_loader_factory;
    network_context->CreateURLLoaderFactory(MakeRequest(&url_loader_factory),
                                            0);
LOG(ERROR) << "1. ChromeNetworkServiceRestartBrowserTest.LoadBasicRequest";
    content::SimpleURLLoaderTestHelper simple_loader_helper;
    std::unique_ptr<content::SimpleURLLoader> simple_loader =
        content::SimpleURLLoader::Create();
LOG(ERROR) << "2. ChromeNetworkServiceRestartBrowserTest.LoadBasicRequest";
    ResourceRequest request;
    request.url = embedded_test_server()->GetURL("/echo");
LOG(ERROR) << "3. ChromeNetworkServiceRestartBrowserTest.LoadBasicRequest";
    simple_loader->DownloadToStringOfUnboundedSizeUntilCrashAndDie(
        request, url_loader_factory.get(), TRAFFIC_ANNOTATION_FOR_TESTS,
        simple_loader_helper.GetCallback());
    LOG(ERROR) << "4. ChromeNetworkServiceRestartBrowserTest.LoadBasicRequest";
    simple_loader_helper.WaitForCallback();

    LOG(ERROR) << "Finished ChromeNetworkServiceRestartBrowserTest.LoadBasicRequest";

    return simple_loader->NetError();
  }

 private:
  base::test::ScopedFeatureList scoped_feature_list_;

  DISALLOW_COPY_AND_ASSIGN(ChromeNetworkServiceRestartBrowserTest);
};

IN_PROC_BROWSER_TEST_F(ChromeNetworkServiceRestartBrowserTest,
                       SimulateNetworkChange) {
  LOG(ERROR) << "Begin ChromeNetworkServiceRestartBrowserTest.SimulateNetworkChange";
  SimulateNetworkChange(mojom::ConnectionType::CONNECTION_3G);
  EXPECT_EQ(1, 2);
  LOG(ERROR) << "Finished ChromeNetworkServiceRestartBrowserTest.SimulateNetworkChange";
}

IN_PROC_BROWSER_TEST_F(ChromeNetworkServiceRestartBrowserTest,
                       SimulateNetworkChange2) {
  LOG(ERROR) << "Begin ChromeNetworkServiceRestartBrowserTest.SimulateNetworkChange2";
  StoragePartition* partition =
      BrowserContext::GetDefaultStoragePartition(browser()->profile());

  mojom::NetworkContext* old_network_context = partition->GetNetworkContext();
  EXPECT_EQ(net::OK, LoadBasicRequest(old_network_context));

  LOG(ERROR) << "1. ChromeNetworkServiceRestartBrowserTest.SimulateNetworkChange2";

  SimulateNetworkChange(mojom::ConnectionType::CONNECTION_3G);
  EXPECT_EQ(1, 2);
  LOG(ERROR) << "Finished ChromeNetworkServiceRestartBrowserTest.SimulateNetworkChange2";
}

// Make sure |StoragePartition::GetNetworkContext()| returns valid interface
// after crash.
IN_PROC_BROWSER_TEST_F(ChromeNetworkServiceRestartBrowserTest,
                       StoragePartitionGetNetworkContext) {
  LOG(ERROR) << "Begin ChromeNetworkServiceRestartBrowserTest.StoragePartitionGetNetworkContext";
  LOG(ERROR) << "base::Process::Current().Pid() = " << base::Process::Current().Pid();
  LOG(ERROR) << "command_line->HasSwitch(kSingleProcess) = " << base::CommandLine::ForCurrentProcess()->HasSwitch(switches::kSingleProcess);
  LOG(ERROR) << "command_line->HasSwitch(kSingleProcessTestsFlag) = " << base::CommandLine::ForCurrentProcess()->HasSwitch(kSingleProcessTestsFlag);

  if (!HasOutOfProcessNetworkService())
    return;

  LOG(ERROR) << "1. ChromeNetworkServiceRestartBrowserTest.StoragePartitionGetNetworkContext";

#if defined(OS_MACOSX)
  // |NetworkServiceTestHelper| doesn't work on browser_tests on macOS.
  return;
#endif
  LOG(ERROR) << "2. ChromeNetworkServiceRestartBrowserTest.StoragePartitionGetNetworkContext";
  StoragePartition* partition =
      BrowserContext::GetDefaultStoragePartition(browser()->profile());

  LOG(ERROR) << "3. ChromeNetworkServiceRestartBrowserTest.StoragePartitionGetNetworkContext";

  mojom::NetworkContext* old_network_context = partition->GetNetworkContext();
  EXPECT_EQ(net::OK, LoadBasicRequest(old_network_context));

  LOG(ERROR) << "4. ChromeNetworkServiceRestartBrowserTest.StoragePartitionGetNetworkContext";

  // Crash the NetworkService process. Existing interfaces should receive error
  // notifications at some point.
  SimulateNetworkServiceCrash();
  LOG(ERROR) << "5. ChromeNetworkServiceRestartBrowserTest.StoragePartitionGetNetworkContext";
  // Flush the interface to make sure the error notification was received.
  partition->FlushNetworkInterfaceForTesting();
  LOG(ERROR) << "6. ChromeNetworkServiceRestartBrowserTest.StoragePartitionGetNetworkContext";
  // |partition->GetNetworkContext()| should return a valid new pointer after
  // crash.
  EXPECT_NE(old_network_context, partition->GetNetworkContext());
  EXPECT_EQ(net::OK, LoadBasicRequest(partition->GetNetworkContext()));
  LOG(ERROR) << "Finished ChromeNetworkServiceRestartBrowserTest.StoragePartitionGetNetworkContext";
}

// Make sure |SystemNetworkContextManager::GetContext()| returns valid interface
// after crash.
IN_PROC_BROWSER_TEST_F(ChromeNetworkServiceRestartBrowserTest,
                       SystemNetworkContextManagerGetContext) {
  LOG(ERROR) << "Begin ChromeNetworkServiceRestartBrowserTest.SystemNetworkContextManagerGetContext";
  LOG(ERROR) << "base::Process::Current().Pid() = " << base::Process::Current().Pid();
  if (!HasOutOfProcessNetworkService())
    return;

#if defined(OS_MACOSX)
  // |NetworkServiceTestHelper| doesn't work on browser_tests on macOS.
  return;
#endif
  SystemNetworkContextManager* system_network_context_manager =
      g_browser_process->system_network_context_manager();

  mojom::NetworkContext* old_network_context =
      system_network_context_manager->GetContext();
  EXPECT_EQ(net::OK, LoadBasicRequest(old_network_context));

  // Crash the NetworkService process. Existing interfaces should receive error
  // notifications at some point.
  SimulateNetworkServiceCrash();
  // Flush the interface to make sure the error notification was received.
  system_network_context_manager->FlushNetworkInterfaceForTesting();

  // |system_network_context_manager->GetContext()| should return a valid new
  // pointer after crash.
  EXPECT_NE(old_network_context, system_network_context_manager->GetContext());
  EXPECT_EQ(net::OK,
            LoadBasicRequest(system_network_context_manager->GetContext()));
  LOG(ERROR) << "Finished ChromeNetworkServiceRestartBrowserTest.SystemNetworkContextManagerGetContext";
}

}  // namespace content
