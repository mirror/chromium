// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/chromeos/net/auto_connect_notifier.h"

#include <memory>

#include "base/memory/ptr_util.h"
#include "chrome/browser/notifications/notification_display_service_tester.h"
#include "chrome/test/base/browser_with_test_window_test.h"
#include "chromeos/network/auto_connect_handler.h"
#include "components/cryptauth/remote_device_test_util.h"
#include "ui/message_center/notification.h"

namespace chromeos {

namespace {

const char kAutoConnectNotificationId[] =
    "cros_auto_connect_notifier_ids.connected_to_network";

}  // namespace

class AutoConnectNotifierTest : public BrowserWithTestWindowTest {
 protected:
  // Nested within AutoConnectNotifierTest for visibility into
  // AutoConnectHandler's constructor.
  class TestAutoConnectHandler : public AutoConnectHandler {
   public:
    // Make AutoConnectHandler::NotifyAutoConnectedToNetwork() public.
    using AutoConnectHandler::NotifyAutoConnectedToNetwork;
  };

  AutoConnectNotifierTest() = default;

  void SetUp() override {
    BrowserWithTestWindowTest::SetUp();

    display_service_ =
        std::make_unique<NotificationDisplayServiceTester>(profile());
    test_auto_connect_handler_ = base::WrapUnique(new TestAutoConnectHandler());

    auto_connect_notifier_ = std::make_unique<AutoConnectNotifier>(
        profile(), test_auto_connect_handler_.get());
  }

  void TearDown() override {
    display_service_.reset();
    BrowserWithTestWindowTest::TearDown();
  }

  std::unique_ptr<NotificationDisplayServiceTester> display_service_;
  std::unique_ptr<TestAutoConnectHandler> test_auto_connect_handler_;

  std::unique_ptr<AutoConnectNotifier> auto_connect_notifier_;

 private:
  DISALLOW_COPY_AND_ASSIGN(AutoConnectNotifierTest);
};

TEST_F(AutoConnectNotifierTest, TestNotificationAfterAutoConnect) {
  EXPECT_FALSE(display_service_->GetNotification(kAutoConnectNotificationId));
  test_auto_connect_handler_->NotifyAutoConnectedToNetwork();

  base::Optional<message_center::Notification> notification =
      display_service_->GetNotification(kAutoConnectNotificationId);
  ASSERT_TRUE(notification);
  EXPECT_EQ(kAutoConnectNotificationId, notification->id());
}

}  // namespace chromeos
