// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <memory>

#include "base/command_line.h"
#include "base/files/file_path.h"
#include "base/files/scoped_temp_dir.h"
#include "base/strings/string_split.h"
#include "base/test/scoped_task_environment.h"
#include "base/threading/thread_task_runner_handle.h"
#include "build/build_config.h"
#include "components/network_session_configurator/browser/network_session_configurator.h"
#include "components/network_session_configurator/common/network_switches.h"
#include "content/network/network_context_impl.h"
#include "content/network/network_service_impl.h"
#include "content/public/common/network_service.mojom.h"
#include "mojo/public/cpp/bindings/interface_request.h"
#include "net/base/cache_type.h"
#include "net/base/net_errors.h"
#include "net/base/test_completion_callback.h"
#include "net/disk_cache/disk_cache.h"
#include "net/http/http_cache.h"
#include "net/http/http_transaction_factory.h"
#include "net/url_request/url_request_context.h"
#include "net/url_request/url_request_context_builder.h"
#include "net/url_request/url_request_job_factory.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/url_constants.h"

namespace content {

namespace {

class NetworkContextImplTest : public testing::Test {
 public:
  NetworkContextImplTest()
      : scoped_task_environment_(
            base::test::ScopedTaskEnvironment::MainThreadType::IO),
        network_service_(NetworkServiceImpl::CreateForTesting()) {}
  ~NetworkContextImplTest() override {}

  std::unique_ptr<NetworkContextImpl> CreateContextWithParams(
      mojom::NetworkContextParamsPtr context_params) {
    return base::MakeUnique<NetworkContextImpl>(
        network_service_.get(), mojo::MakeRequest(&network_context_ptr_),
        std::move(context_params));
  }

  // Searches through |backend|'s stats to discover its type. Only supports
  // blockfile and simple caches.
  net::URLRequestContextBuilder::HttpCacheParams::Type GetBackendType(
      disk_cache::Backend* backend) {
    base::StringPairs stats;
    backend->GetStats(&stats);
    for (const auto& pair : stats) {
      if (pair.first != "Cache type")
        continue;

      if (pair.second == "Simple Cache")
        return net::URLRequestContextBuilder::HttpCacheParams::DISK_SIMPLE;
      if (pair.second == "Blockfile Cache")
        return net::URLRequestContextBuilder::HttpCacheParams::DISK_BLOCKFILE;
      break;
    }

    NOTREACHED();
    return net::URLRequestContextBuilder::HttpCacheParams::IN_MEMORY;
  }

 private:
  base::test::ScopedTaskEnvironment scoped_task_environment_;
  std::unique_ptr<NetworkServiceImpl> network_service_;
  // Stores the NetworkContextImplPtr of the most recently created
  // NetworkContextImpl, since destroying this before the NetworkContextImpl
  // itself triggers deletion of the NetworkContextImpl. These tests are
  // probably fine anyways, since the message loop must be spun for that to
  // happen.
  mojom::NetworkContextPtr network_context_ptr_;
};

TEST_F(NetworkContextImplTest, DisableDataUrlSupport) {
  mojom::NetworkContextParamsPtr context_params =
      mojom::NetworkContextParams::New();
  context_params->enable_data_url_support = false;
  std::unique_ptr<NetworkContextImpl> network_context =
      CreateContextWithParams(std::move(context_params));
  EXPECT_FALSE(
      network_context->url_request_context()->job_factory()->IsHandledProtocol(
          url::kDataScheme));
}

TEST_F(NetworkContextImplTest, EnableDataUrlSupport) {
  mojom::NetworkContextParamsPtr context_params =
      mojom::NetworkContextParams::New();
  context_params->enable_data_url_support = true;
  std::unique_ptr<NetworkContextImpl> network_context =
      CreateContextWithParams(std::move(context_params));
  EXPECT_TRUE(
      network_context->url_request_context()->job_factory()->IsHandledProtocol(
          url::kDataScheme));
}

TEST_F(NetworkContextImplTest, DisableFileUrlSupport) {
  mojom::NetworkContextParamsPtr context_params =
      mojom::NetworkContextParams::New();
  context_params->enable_file_url_support = false;
  std::unique_ptr<NetworkContextImpl> network_context =
      CreateContextWithParams(std::move(context_params));
  EXPECT_FALSE(
      network_context->url_request_context()->job_factory()->IsHandledProtocol(
          url::kFileScheme));
}

#if !BUILDFLAG(DISABLE_FILE_SUPPORT)
TEST_F(NetworkContextImplTest, EnableFileUrlSupport) {
  mojom::NetworkContextParamsPtr context_params =
      mojom::NetworkContextParams::New();
  context_params->enable_file_url_support = true;
  std::unique_ptr<NetworkContextImpl> network_context =
      CreateContextWithParams(std::move(context_params));
  EXPECT_TRUE(
      network_context->url_request_context()->job_factory()->IsHandledProtocol(
          url::kFileScheme));
}
#endif  // !BUILDFLAG(DISABLE_FILE_SUPPORT)

TEST_F(NetworkContextImplTest, DisableFtpUrlSupport) {
  mojom::NetworkContextParamsPtr context_params =
      mojom::NetworkContextParams::New();
  context_params->enable_ftp_url_support = false;
  std::unique_ptr<NetworkContextImpl> network_context =
      CreateContextWithParams(std::move(context_params));
  EXPECT_FALSE(
      network_context->url_request_context()->job_factory()->IsHandledProtocol(
          url::kFtpScheme));
}

#if !BUILDFLAG(DISABLE_FTP_SUPPORT)
TEST_F(NetworkContextImplTest, EnableFtpUrlSupport) {
  mojom::NetworkContextParamsPtr context_params =
      mojom::NetworkContextParams::New();
  context_params->enable_ftp_url_support = true;
  std::unique_ptr<NetworkContextImpl> network_context =
      CreateContextWithParams(std::move(context_params));
  EXPECT_TRUE(
      network_context->url_request_context()->job_factory()->IsHandledProtocol(
          url::kFtpScheme));
}
#endif  // !BUILDFLAG(DISABLE_FTP_SUPPORT)

TEST_F(NetworkContextImplTest, NoCache) {
  mojom::NetworkContextParamsPtr context_params =
      mojom::NetworkContextParams::New();
  context_params->http_cache_enabled = false;
  std::unique_ptr<NetworkContextImpl> network_context =
      CreateContextWithParams(std::move(context_params));
  EXPECT_FALSE(network_context->url_request_context()
                   ->http_transaction_factory()
                   ->GetCache());
}

TEST_F(NetworkContextImplTest, MemoryCache) {
  mojom::NetworkContextParamsPtr context_params =
      mojom::NetworkContextParams::New();
  context_params->http_cache_enabled = true;
  std::unique_ptr<NetworkContextImpl> network_context =
      CreateContextWithParams(std::move(context_params));
  net::HttpCache* cache = network_context->url_request_context()
                              ->http_transaction_factory()
                              ->GetCache();
  ASSERT_TRUE(cache);

  disk_cache::Backend* backend = nullptr;
  net::TestCompletionCallback callback;
  int rv = cache->GetBackend(&backend, callback.callback());
  EXPECT_EQ(net::OK, callback.GetResult(rv));
  ASSERT_TRUE(backend);

  EXPECT_EQ(net::MEMORY_CACHE, backend->GetCacheType());
}

TEST_F(NetworkContextImplTest, DiskCache) {
  mojom::NetworkContextParamsPtr context_params =
      mojom::NetworkContextParams::New();
  context_params->http_cache_enabled = true;

  base::ScopedTempDir temp_dir;
  ASSERT_TRUE(temp_dir.CreateUniqueTempDir());
  context_params->http_cache_path = temp_dir.GetPath();

  std::unique_ptr<NetworkContextImpl> network_context =
      CreateContextWithParams(std::move(context_params));
  net::HttpCache* cache = network_context->url_request_context()
                              ->http_transaction_factory()
                              ->GetCache();
  ASSERT_TRUE(cache);

  disk_cache::Backend* backend = nullptr;
  net::TestCompletionCallback callback;
  int rv = cache->GetBackend(&backend, callback.callback());
  EXPECT_EQ(net::OK, callback.GetResult(rv));
  ASSERT_TRUE(backend);

  EXPECT_EQ(net::DISK_CACHE, backend->GetCacheType());
  EXPECT_EQ(network_session_configurator::ChooseCacheType(
                *base::CommandLine::ForCurrentProcess()),
            GetBackendType(backend));
}

// This makes sure that network_session_configurator::ChooseCacheType is
// connected to NetworkContextImpl.
TEST_F(NetworkContextImplTest, SimpleCache) {
  base::CommandLine::ForCurrentProcess()->AppendSwitchASCII(
      switches::kUseSimpleCacheBackend, "on");
  mojom::NetworkContextParamsPtr context_params =
      mojom::NetworkContextParams::New();
  context_params->http_cache_enabled = true;

  base::ScopedTempDir temp_dir;
  ASSERT_TRUE(temp_dir.CreateUniqueTempDir());
  context_params->http_cache_path = temp_dir.GetPath();

  std::unique_ptr<NetworkContextImpl> network_context =
      CreateContextWithParams(std::move(context_params));
  net::HttpCache* cache = network_context->url_request_context()
                              ->http_transaction_factory()
                              ->GetCache();
  ASSERT_TRUE(cache);

  disk_cache::Backend* backend = nullptr;
  net::TestCompletionCallback callback;
  int rv = cache->GetBackend(&backend, callback.callback());
  EXPECT_EQ(net::OK, callback.GetResult(rv));
  ASSERT_TRUE(backend);

  base::StringPairs stats;
  backend->GetStats(&stats);
  EXPECT_EQ(net::URLRequestContextBuilder::HttpCacheParams::DISK_SIMPLE,
            GetBackendType(backend));
}

}  // namespace

}  // namespace content
