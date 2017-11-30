// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/signin/ios/browser/delay_callback_helper.h"

#include "base/bind.h"
#include "base/run_loop.h"
#include "base/test/scoped_task_environment.h"
#include "net/base/mock_network_change_notifier.h"
#include "testing/gtest/include/gtest/gtest.h"

// A test fixture to test DelayCallbackHelper.
class DelayCallbackHelperTest : public testing::Test {
 public:
  void CallbackFunction() { callback_was_called_ = true; }

 protected:
  DelayCallbackHelperTest() : callback_was_called_(false) {}

  bool callback_was_called_;
  base::test::ScopedTaskEnvironment scoped_task_environment_;
  net::test::MockNetworkChangeNotifier network_change_notifier_;
  DelayCallbackHelper delay_callback_helper_;
};

TEST_F(DelayCallbackHelperTest, CallbackInvokedImmediately) {
  network_change_notifier_.SetConnectionType(
      net::NetworkChangeNotifier::ConnectionType::CONNECTION_WIFI);
  delay_callback_helper_.HandleCallback(base::Bind(
      &DelayCallbackHelperTest::CallbackFunction, base::Unretained(this)));
  EXPECT_TRUE(callback_was_called_);
}

TEST_F(DelayCallbackHelperTest, CallbackInvokedLater) {
  network_change_notifier_.SetConnectionType(
      net::NetworkChangeNotifier::ConnectionType::CONNECTION_NONE);
  delay_callback_helper_.HandleCallback(base::Bind(
      &DelayCallbackHelperTest::CallbackFunction, base::Unretained(this)));
  EXPECT_FALSE(callback_was_called_);

  network_change_notifier_.SetConnectionType(
      net::NetworkChangeNotifier::ConnectionType::CONNECTION_WIFI);
  network_change_notifier_.NotifyObserversOfConnectionTypeChangeForTests(
      net::NetworkChangeNotifier::ConnectionType::CONNECTION_WIFI);
  scoped_task_environment_.RunUntilIdle();
  EXPECT_TRUE(callback_was_called_);
}
