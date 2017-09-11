// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/network/network_service_ssl_config_service.h"

#include "base/test/scoped_task_environment.h"
#include "content/network/network_context.h"
#include "content/network/network_service_impl.h"
#include "content/public/common/network_service.mojom.h"
#include "content/public/common/ssl_config.mojom.h"
#include "mojo/public/cpp/bindings/interface_request.h"
#include "net/ssl/ssl_config.h"
#include "net/ssl/ssl_config_service.h"
#include "net/url_request/url_request_context.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace content {
namespace {

class TestSSLConfigServiceObserver : public net::SSLConfigService::Observer {
 public:
  explicit TestSSLConfigServiceObserver(
      net::SSLConfigService* ssl_config_service)
      : ssl_config_service_(ssl_config_service) {
    ssl_config_service_->AddObserver(this);
  }
  ~TestSSLConfigServiceObserver() override {
    ssl_config_service_->RemoveObserver(this);
  }

  // net::SSLConfigService::Observer implementation:
  void OnSSLConfigChanged() override {
    ++observed_changes_;
    ssl_config_service_->GetSSLConfig(&ssl_config_during_change_);
    run_loop_.Quit();
  }

  void WaitForChange() { run_loop_.Run(); }

  const net::SSLConfig& ssl_config_during_change() const {
    return ssl_config_during_change_;
  }

  size_t observed_changes() const { return observed_changes_; }

 private:
  net::SSLConfigService* const ssl_config_service_;
  size_t observed_changes_ = 0;
  net::SSLConfig ssl_config_during_change_;
  base::RunLoop run_loop_;
};

class NetworkServiceSSLConfigServiceTest : public testing::Test {
 public:
  NetworkServiceSSLConfigServiceTest()
      : scoped_task_environment_(
            base::test::ScopedTaskEnvironment::MainThreadType::IO),
        network_service_(NetworkServiceImpl::CreateForTesting()) {}

 protected:
  base::test::ScopedTaskEnvironment scoped_task_environment_;
  std::unique_ptr<NetworkServiceImpl> network_service_;
};

// Check that the default SSLConfig is used by default.
TEST_F(NetworkServiceSSLConfigServiceTest, Default) {
  mojom::NetworkContextPtr network_context_ptr;
  NetworkContext network_context(network_service_.get(),
                                 mojo::MakeRequest(&network_context_ptr),
                                 mojom::NetworkContextParams::New());
  net::SSLConfig ssl_config;
  network_context.url_request_context()->ssl_config_service()->GetSSLConfig(
      &ssl_config);
  EXPECT_TRUE(net::SSLConfigService::UserConfigsAreEqualForTesting(
      ssl_config, net::SSLConfig()));
}

TEST_F(NetworkServiceSSLConfigServiceTest, InitialConfigNoSSLConfigClient) {
  mojom::NetworkContextPtr network_context_ptr;
  mojom::NetworkContextParamsPtr network_context_params =
      mojom::NetworkContextParams::New();

  // Disable a cipher suite, to make initial configuration
  net::SSLConfig initial_ssl_config;
  initial_ssl_config.disabled_cipher_suites.push_back(0x0004);
  network_context_params->initial_ssl_config = initial_ssl_config;

  NetworkContext network_context(network_service_.get(),
                                 mojo::MakeRequest(&network_context_ptr),
                                 std::move(network_context_params));
  net::SSLConfig ssl_config;
  network_context.url_request_context()->ssl_config_service()->GetSSLConfig(
      &ssl_config);
  EXPECT_TRUE(net::SSLConfigService::UserConfigsAreEqualForTesting(
      initial_ssl_config, ssl_config));
}

TEST_F(NetworkServiceSSLConfigServiceTest, InitialConfigWithSSLConfigClient) {
  mojom::NetworkContextPtr network_context_ptr;
  mojom::NetworkContextParamsPtr network_context_params =
      mojom::NetworkContextParams::New();

  // Disable a cipher suite, to make initial configuration
  net::SSLConfig initial_ssl_config;
  initial_ssl_config.disabled_cipher_suites.push_back(0x0004);
  network_context_params->initial_ssl_config = initial_ssl_config;

  mojom::SSLConfigClientPtr ssl_config_client;
  network_context_params->ssl_config_client =
      mojo::MakeRequest(&ssl_config_client);

  NetworkContext network_context(network_service_.get(),
                                 mojo::MakeRequest(&network_context_ptr),
                                 std::move(network_context_params));
  net::SSLConfigService* ssl_config_service =
      network_context.url_request_context()->ssl_config_service();
  net::SSLConfig ssl_config;
  ssl_config_service->GetSSLConfig(&ssl_config);
  EXPECT_TRUE(net::SSLConfigService::UserConfigsAreEqualForTesting(
      initial_ssl_config, ssl_config));

  TestSSLConfigServiceObserver observer(ssl_config_service);

  net::SSLConfig new_ssl_config;
  new_ssl_config.disabled_cipher_suites.push_back(0xC002);
  // Simple sanity check.
  EXPECT_FALSE(net::SSLConfigService::UserConfigsAreEqualForTesting(
      initial_ssl_config, new_ssl_config));

  ssl_config_client->UpdateSSLConfig(new_ssl_config);
  observer.WaitForChange();
  EXPECT_EQ(1u, observer.observed_changes());
  EXPECT_TRUE(net::SSLConfigService::UserConfigsAreEqualForTesting(
      new_ssl_config, observer.ssl_config_during_change()));

  ssl_config_service->GetSSLConfig(&ssl_config);
  EXPECT_TRUE(net::SSLConfigService::UserConfigsAreEqualForTesting(
      new_ssl_config, ssl_config));
}

}  // namespace
}  // namespace content
