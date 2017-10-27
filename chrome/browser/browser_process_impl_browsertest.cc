// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/browser_process_impl.h"

#include "base/logging.h"
#include "base/macros.h"
#include "base/message_loop/message_loop.h"
#include "base/run_loop.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "content/public/common/network_connection_tracker.h"
#include "content/public/test/browser_test_utils.h"
#include "net/base/mock_network_change_notifier.h"

namespace content {

namespace {

class TestNetworkConnectionObserver
    : public NetworkConnectionTracker::NetworkConnectionObserver {
 public:
  explicit TestNetworkConnectionObserver(NetworkConnectionTracker* tracker)
      : num_notifications_(0),
        tracker_(tracker),
        run_loop_(std::make_unique<base::RunLoop>()),
        connection_type_(mojom::ConnectionType::CONNECTION_UNKNOWN) {
    tracker_->AddNetworkConnectionObserver(this);
  }

  ~TestNetworkConnectionObserver() override {
    tracker_->RemoveNetworkConnectionObserver(this);
  }

  // Helper to synchronously get connection type from NetworkConnectionTracker.
  mojom::ConnectionType GetConnectionTypeSync() {
    mojom::ConnectionType type;
    base::RunLoop run_loop;
    bool sync = tracker_->GetConnectionType(
        &type,
        base::Bind(&TestNetworkConnectionObserver::GetConnectionTypeCallback,
                   &run_loop, &type));
    if (!sync)
      run_loop.Run();
    return type;
  }

  // NetworkConnectionObserver implementation:
  void OnConnectionChanged(mojom::ConnectionType type) override {
    EXPECT_EQ(type, GetConnectionTypeSync());

    num_notifications_++;
    connection_type_ = type;
    run_loop_->Quit();
  }

  size_t num_notifications() const { return num_notifications_; }
  void WaitForNotification() {
    run_loop_->Run();
    run_loop_.reset(new base::RunLoop());
  }

  mojom::ConnectionType connection_type() const { return connection_type_; }

 private:
  static void GetConnectionTypeCallback(base::RunLoop* run_loop,
                                        mojom::ConnectionType* out,
                                        mojom::ConnectionType type) {
    *out = type;
    run_loop->Quit();
  }

  size_t num_notifications_;
  NetworkConnectionTracker* tracker_;
  std::unique_ptr<base::RunLoop> run_loop_;
  mojom::ConnectionType connection_type_;

  DISALLOW_COPY_AND_ASSIGN(TestNetworkConnectionObserver);
};

class BrowserProcessImplTest : public InProcessBrowserTest {
 public:
  BrowserProcessImplTest() {}
  ~BrowserProcessImplTest() override {}
};

// Basic test to make sure NetworkConnectionTracker is set up.
IN_PROC_BROWSER_TEST_F(BrowserProcessImplTest, NetworkConnectionTracker) {
  // Use an in-process mock network change notifier to simulate connection type
  // changes. Note: This needs to be changed if NetworkService is running out of
  // process.
  net::test::ScopedMockNetworkChangeNotifier mock_network_change_notifier;

  NetworkConnectionTracker* tracker =
      g_browser_process->network_connection_tracker();
  EXPECT_NE(nullptr, tracker);
  mock_network_change_notifier.mock_network_change_notifier()
      ->SetConnectionType(net::NetworkChangeNotifier::CONNECTION_3G);
  TestNetworkConnectionObserver network_connection_observer(tracker);
  EXPECT_EQ(mojom::ConnectionType::CONNECTION_3G,
            network_connection_observer.GetConnectionTypeSync());
  mock_network_change_notifier.mock_network_change_notifier()
      ->NotifyObserversOfNetworkChangeForTests(
          net::NetworkChangeNotifier::ConnectionType::CONNECTION_WIFI);

  network_connection_observer.WaitForNotification();
  EXPECT_EQ(mojom::ConnectionType::CONNECTION_WIFI,
            network_connection_observer.connection_type());
  base::RunLoop().RunUntilIdle();
  EXPECT_EQ(1u, network_connection_observer.num_notifications());
}

}  // namespace

}  // namespace content
