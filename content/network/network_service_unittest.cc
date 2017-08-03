// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <memory>

#include "base/run_loop.h"
#include "base/test/scoped_task_environment.h"
#include "base/threading/thread_task_runner_handle.h"
#include "content/network/network_context.h"
#include "content/network/network_service_impl.h"
#include "content/public/common/network_service.mojom.h"
#include "content/public/common/url_loader.mojom.h"
#include "content/public/common/url_loader_factory.mojom.h"
#include "content/public/test/test_url_loader_client.h"
#include "net/traffic_annotation/network_traffic_annotation_test_helper.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace content {

namespace {

class NetworkServiceTest : public testing::Test {
 public:
  NetworkServiceTest()
      : scoped_task_environment_(
            base::test::ScopedTaskEnvironment::MainThreadType::IO),
        service_(NetworkServiceImpl::CreateForTesting()) {}
  ~NetworkServiceTest() override {}

  NetworkService* service() const { return service_.get(); }

  void DestroyService() { service_.reset(); }

 private:
  base::test::ScopedTaskEnvironment scoped_task_environment_;
  std::unique_ptr<NetworkService> service_;
};

// Test shutdown in the case a NetworkContext is destroyed before the
// NetworkService.
TEST_F(NetworkServiceTest, CreateAndDestroyContext) {
  mojom::NetworkContextPtr network_context;
  mojom::NetworkContextParamsPtr context_params =
      mojom::NetworkContextParams::New();

  service()->CreateNetworkContext(mojo::MakeRequest(&network_context),
                                  std::move(context_params));
  network_context.reset();
  // Make sure the NetworkContext is destroyed.
  base::RunLoop().RunUntilIdle();
}

// Test shutdown in the case there is still a live NetworkContext when the
// NetworkService is destroyed. The service should destroy the NetworkContext
// itself.
TEST_F(NetworkServiceTest, DestroyingServiceDestroysContext) {
  mojom::NetworkContextPtr network_context;
  mojom::NetworkContextParamsPtr context_params =
      mojom::NetworkContextParams::New();

  service()->CreateNetworkContext(mojo::MakeRequest(&network_context),
                                  std::move(context_params));
  base::RunLoop run_loop;
  network_context.set_connection_error_handler(run_loop.QuitClosure());
  DestroyService();

  // Destroying the service should destroy the context, causing a connection
  // error.
  run_loop.Run();
}

// Test shutdown in the case a NetworkContext is destroyed before the
// NetworkService.
TEST_F(NetworkServiceTest, CreateAndDestroyContextMef) {
  mojom::NetworkContextPtr network_context;
  mojom::NetworkContextParamsPtr context_params =
      mojom::NetworkContextParams::New();

  service()->CreateNetworkContext(mojo::MakeRequest(&network_context),
                                  std::move(context_params));

  mojom::URLLoaderFactoryPtr url_loader_factory;
  network_context->CreateURLLoaderFactory(MakeRequest(&url_loader_factory), 0);
  TestURLLoaderClient client;
  ResourceRequest request;
  request.url = GURL("Zhttps://www.google.com");
  request.method = "GET";

  mojom::URLLoaderPtr loader;
  url_loader_factory->CreateLoaderAndStart(
      mojo::MakeRequest(&loader), 2, 1, content::mojom::kURLLoadOptionNone,
      request, client.CreateInterfacePtr(),
      net::MutableNetworkTrafficAnnotationTag(TRAFFIC_ANNOTATION_FOR_TESTS));

  client.RunUntilResponseReceived();
  // SyncLoadResult result;

  // url_loader_factory->SyncLoad(1,2, request, &result);

  network_context.reset();
  // Make sure the NetworkContext is destroyed.
  base::RunLoop().RunUntilIdle();
}

}  // namespace

}  // namespace content
