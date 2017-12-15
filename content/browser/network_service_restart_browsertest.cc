// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/test/scoped_feature_list.h"
#include "build/build_config.h"
#include "content/browser/storage_partition_impl.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/network_service_instance.h"
#include "content/public/browser/web_contents.h"
#include "content/public/common/content_features.h"
#include "content/public/common/network_service.mojom.h"
#include "content/public/test/browser_test_utils.h"
#include "content/public/test/content_browser_test.h"
#include "content/public/test/content_browser_test_utils.h"
#include "content/shell/browser/shell.h"

namespace content {

namespace {

mojom::NetworkContextPtr CreateNetworkContext() {
  mojom::NetworkContextPtr network_context;
  mojom::NetworkContextParamsPtr context_params =
      mojom::NetworkContextParams::New();
  GetNetworkService()->CreateNetworkContext(mojo::MakeRequest(&network_context),
                                            std::move(context_params));
  return network_context;
}

}  // namespace

// This test source has been excluded from Android as Android doesn't have
// out-of-process Network Service.
class NetworkServiceRestartBrowserTest
    : public ContentBrowserTest,
      public StoragePartition::NetworkContextObserver {
 public:
  NetworkServiceRestartBrowserTest() {
    scoped_feature_list_.InitAndEnableFeature(features::kNetworkService);
    EXPECT_TRUE(embedded_test_server()->Start());
  }

  // StoragePartition::NetworkContextObserver overrides:
  void OnConnectionError() override {
    if (connection_error_closure_)
      std::move(connection_error_closure_).Run();
  }

  void SetConnectionErrorClosure(base::OnceClosure closure) {
    DCHECK(!connection_error_closure_);
    connection_error_closure_ = std::move(closure);
  }

  GURL GetTestURL() const {
    // Use '/echoheader' instead of '/echo' to avoid a disk_cache bug.
    // See https://crbug.com/792255.
    return embedded_test_server()->GetURL("/echoheader");
  }

 private:
  base::test::ScopedFeatureList scoped_feature_list_;
  base::OnceClosure connection_error_closure_;

  DISALLOW_COPY_AND_ASSIGN(NetworkServiceRestartBrowserTest);
};

IN_PROC_BROWSER_TEST_F(NetworkServiceRestartBrowserTest,
                       NetworkServiceProcessRecovery) {
  mojom::NetworkContextPtr network_context = CreateNetworkContext();
  EXPECT_EQ(net::OK, LoadBasicRequest(network_context.get(), GetTestURL()));
  EXPECT_TRUE(network_context.is_bound());
  EXPECT_FALSE(network_context.encountered_error());

  // Crash the NetworkService process. Existing interfaces should receive error
  // notifications at some point.
  SimulateNetworkServiceCrash();
  // |network_context| will receive an error notification, but it's not
  // guaranteed to have arrived at this point. Flush the pointer to make sure
  // the notification has been received.
  network_context.FlushForTesting();
  EXPECT_TRUE(network_context.is_bound());
  EXPECT_TRUE(network_context.encountered_error());
  // Make sure we could get |net::ERR_FAILED| with an invalid |network_context|.
  EXPECT_EQ(net::ERR_FAILED,
            LoadBasicRequest(network_context.get(), GetTestURL()));

  // NetworkService should restart automatically and return valid interface.
  mojom::NetworkContextPtr network_context2 = CreateNetworkContext();
  EXPECT_EQ(net::OK, LoadBasicRequest(network_context2.get(), GetTestURL()));
  EXPECT_TRUE(network_context2.is_bound());
  EXPECT_FALSE(network_context2.encountered_error());
}

// Make sure |StoragePartitionImpl::GetNetworkContext()| returns valid interface
// after crash.
IN_PROC_BROWSER_TEST_F(NetworkServiceRestartBrowserTest,
                       StoragePartitionImplGetNetworkContext) {
  StoragePartitionImpl* partition = static_cast<StoragePartitionImpl*>(
      BrowserContext::GetDefaultStoragePartition(
          shell()->web_contents()->GetBrowserContext()));
  partition->AddNetworkContextObserver(this);

  mojom::NetworkContext* old_network_context = partition->GetNetworkContext();
  EXPECT_EQ(net::OK, LoadBasicRequest(old_network_context, GetTestURL()));

  base::RunLoop run_loop;
  SetConnectionErrorClosure(run_loop.QuitClosure());
  // Crash the NetworkService process. Existing interfaces should receive error
  // notifications at some point.
  SimulateNetworkServiceCrash();
  // Flush the interface to make sure the error notification was received.
  partition->FlushNetworkInterfaceForTesting();
  // Wait for connection error notification.
  run_loop.Run();

  // |partition->GetNetworkContext()| should return a valid new pointer after
  // crash.
  EXPECT_NE(old_network_context, partition->GetNetworkContext());
  EXPECT_EQ(net::OK,
            LoadBasicRequest(partition->GetNetworkContext(), GetTestURL()));
  partition->RemoveNetworkContextObserver(this);
}

// Make sure |StoragePartitionImpl::GetNetworkContext()| returns valid interface
// after multiple crashes.
IN_PROC_BROWSER_TEST_F(NetworkServiceRestartBrowserTest,
                       StoragePartitionImplGetNetworkContextMultipleCrash) {
  StoragePartitionImpl* partition = static_cast<StoragePartitionImpl*>(
      BrowserContext::GetDefaultStoragePartition(
          shell()->web_contents()->GetBrowserContext()));
  partition->AddNetworkContextObserver(this);

  EXPECT_EQ(net::OK,
            LoadBasicRequest(partition->GetNetworkContext(), GetTestURL()));

  base::RunLoop run_loop;
  SetConnectionErrorClosure(run_loop.QuitClosure());
  // Crash the NetworkService process. Existing interfaces should receive error
  // notifications at some point.
  SimulateNetworkServiceCrash();
  // Flush the interface to make sure the error notification was received.
  partition->FlushNetworkInterfaceForTesting();
  // Wait for connection error notification.
  run_loop.Run();

  // |partition->GetNetworkContext()| should still return a valid pointer.
  EXPECT_EQ(net::OK,
            LoadBasicRequest(partition->GetNetworkContext(), GetTestURL()));

  base::RunLoop run_loop2;
  SetConnectionErrorClosure(run_loop2.QuitClosure());
  // Crash the NetworkService process again.
  SimulateNetworkServiceCrash();
  // Flush the interface to make sure the error notification was received.
  partition->FlushNetworkInterfaceForTesting();
  // Wait for connection error notification.
  run_loop2.Run();

  // |partition->GetNetworkContext()| should still return a valid pointer.
  EXPECT_EQ(net::OK,
            LoadBasicRequest(partition->GetNetworkContext(), GetTestURL()));
  partition->RemoveNetworkContextObserver(this);
}

// Make sure |StoragePartitionImpl| returns new pointers during
// |NetworkContextObserver::OnConnectionError()|.
IN_PROC_BROWSER_TEST_F(NetworkServiceRestartBrowserTest,
                       StoragePartitionImplNetworkContextObserver) {
  StoragePartitionImpl* partition = static_cast<StoragePartitionImpl*>(
      BrowserContext::GetDefaultStoragePartition(
          shell()->web_contents()->GetBrowserContext()));
  partition->AddNetworkContextObserver(this);

  mojom::NetworkContext* old_network_context = partition->GetNetworkContext();
  mojom::URLLoaderFactory* old_loader_factory =
      partition->GetURLLoaderFactoryForBrowserProcess();

  EXPECT_EQ(net::OK, LoadBasicRequest(old_network_context, GetTestURL()));
  EXPECT_EQ(net::OK, LoadBasicRequest(old_loader_factory, GetTestURL()));

  base::RunLoop run_loop;
  SetConnectionErrorClosure(base::BindOnce(
      [](mojom::NetworkContext* old_context,
         mojom::URLLoaderFactory* old_loader,
         StoragePartitionImpl* partition_impl, base::OnceClosure callback) {
        // There is no guarantee that the system won't reuse the same address.
        // Should figure out another way to test if it appears to be flaky.
        EXPECT_NE(old_context, partition_impl->GetNetworkContext());
        EXPECT_NE(old_loader,
                  partition_impl->GetURLLoaderFactoryForBrowserProcess());

        // TODO(chongz): Hook up with |network_service_instance|'s error
        // notification to make sure new pointers above are created from the
        // restarted |network_service_instance|.

        std::move(callback).Run();
      },
      base::Unretained(old_network_context),
      base::Unretained(old_loader_factory), base::Unretained(partition),
      run_loop.QuitClosure()));
  // Crash the NetworkService process. Existing interfaces should receive error
  // notifications at some point.
  SimulateNetworkServiceCrash();
  // Wait for connection error notification.
  run_loop.Run();

  partition->RemoveNetworkContextObserver(this);
}

}  // namespace content
