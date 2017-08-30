// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/network/network_change_manager_impl.h"

#include <algorithm>

#include "base/macros.h"
#include "base/run_loop.h"
#include "base/test/scoped_feature_list.h"
#include "content/network/network_change_manager_client_impl.h"
#include "content/network/network_service_impl.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/common/content_features.h"
#include "content/public/common/network_change_manager.mojom.h"
#include "content/public/test/test_browser_thread_bundle.h"
#include "net/base/network_change_notifier.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace content {

namespace {

class TestObserver
    : public NetworkChangeManagerClientImpl::NetworkChangeObserver {
 public:
  TestObserver() {
    NetworkChangeManagerClientImpl::AddNetworkChangeObserver(this);
  }
  ~TestObserver() override {
    NetworkChangeManagerClientImpl::RemoveNetworkChangeObserver(this);
  }

  void OnNetworkChanged(mojom::ConnectionType type) override {
    run_loop_.Quit();
  }

  void WaitForNotification() { run_loop_.Run(); }

 private:
  base::RunLoop run_loop_;

  DISALLOW_COPY_AND_ASSIGN(TestObserver);
};

}  // namespace

class NetworkChangeManagerImplTest : public testing::Test {
 public:
  NetworkChangeManagerImplTest()
      : thread_bundle_(TestBrowserThreadBundle::REAL_IO_THREAD),
        network_change_notifier_(net::NetworkChangeNotifier::Create()),
        network_service_(NetworkServiceImpl::CreateForTesting()) {
    feature_list_.InitAndEnableFeature(features::kNetworkService);
  }
  ~NetworkChangeManagerImplTest() override {}

  void TearDownService() { network_service_.reset(); }

  void Initialize() {
    base::RunLoop run_loop;
    BrowserThread::PostTask(
        BrowserThread::UI, FROM_HERE,
        base::BindOnce(
            &NetworkChangeManagerImplTest::InitializeOnBrowserUIThread,
            base::Unretained(this), &run_loop));
    run_loop.Run();
  }

  bool connection_error_seen() const { return connection_error_seen_; }

  mojom::NetworkService* network_service() { return network_service_.get(); }

 private:
  void InitializeOnBrowserUIThread(base::RunLoop* run_loop) {
    NetworkChangeManagerClientImpl::Initialize(network_service_.get());
    run_loop->Quit();
  }

  void OnConnectionError() { connection_error_seen_ = true; }

  bool connection_error_seen_ = false;

  TestBrowserThreadBundle thread_bundle_;
  base::test::ScopedFeatureList feature_list_;
  std::unique_ptr<net::NetworkChangeNotifier> network_change_notifier_;
  std::unique_ptr<NetworkServiceImpl> network_service_;

  DISALLOW_COPY_AND_ASSIGN(NetworkChangeManagerImplTest);
};

TEST_F(NetworkChangeManagerImplTest, ObserverNotified) {
  Initialize();
  TestObserver observer;
  ASSERT_EQ(mojom::ConnectionType::CONNECTION_UNKNOWN,
            NetworkChangeManagerClientImpl::GetConnectionType());
  ASSERT_EQ(mojom::ConnectionSubtype::SUBTYPE_UNKNOWN,
            NetworkChangeManagerClientImpl::GetConnectionSubtype());

  // Simulate a network change.
  net::NetworkChangeNotifier::NotifyObserversOfNetworkChangeForTests(
      net::NetworkChangeNotifier::CONNECTION_3G);
  observer.WaitForNotification();
  ASSERT_EQ(mojom::ConnectionType::CONNECTION_3G,
            NetworkChangeManagerClientImpl::GetConnectionType());
  ASSERT_EQ(mojom::ConnectionSubtype::SUBTYPE_UNKNOWN,
            NetworkChangeManagerClientImpl::GetConnectionSubtype());
}

}  // namespace content
