// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/views/restart_notification/restart_notification_controller.h"

#include <memory>

#include "base/macros.h"
#include "base/values.h"
#include "chrome/browser/upgrade_detector.h"
#include "chrome/common/pref_names.h"
#include "chrome/test/base/scoped_testing_local_state.h"
#include "chrome/test/base/testing_browser_process.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace {

// A delegate interface for handling the actions taken by the controller.
class ControllerDelegate {
 public:
  virtual ~ControllerDelegate() = default;
  virtual void ShowRestartRecommendedBubble() = 0;
  virtual void CloseRestartRecommendedBubble() = 0;
  virtual void ShowRestartRequiredDialog() = 0;
  virtual void CloseRestartRequiredDialog() = 0;

 protected:
  ControllerDelegate() = default;
};

// A fake controller that asks a delegate to do work.
class FakeRestartNotificationController : public RestartNotificationController {
 public:
  FakeRestartNotificationController(UpgradeDetector* upgrade_detector,
                                    ControllerDelegate* delegate)
      : RestartNotificationController(upgrade_detector), delegate_(delegate) {}

 private:
  void ShowRestartRecommendedBubble() override {
    delegate_->ShowRestartRecommendedBubble();
  }

  void CloseRestartRecommendedBubble() override {
    delegate_->CloseRestartRecommendedBubble();
  }

  void ShowRestartRequiredDialog() override {
    delegate_->ShowRestartRequiredDialog();
  }

  void CloseRestartRequiredDialog() override {
    delegate_->CloseRestartRequiredDialog();
  }

  ControllerDelegate* delegate_;

  DISALLOW_COPY_AND_ASSIGN(FakeRestartNotificationController);
};

// A mock delegate for testing.
class MockControllerDelegate : public ControllerDelegate {
 public:
  MOCK_METHOD0(ShowRestartRecommendedBubble, void());
  MOCK_METHOD0(CloseRestartRecommendedBubble, void());
  MOCK_METHOD0(ShowRestartRequiredDialog, void());
  MOCK_METHOD0(CloseRestartRequiredDialog, void());
};

// A fake UpgradeDetector.
class FakeUpgradeDetector : public UpgradeDetector {
 public:
  FakeUpgradeDetector() = default;

  // Sets the annoyance level to |level| and broadcasts the change to all
  // observers.
  void BroadcastLevelChange(UpgradeNotificationAnnoyanceLevel level) {
    set_upgrade_notification_stage(level);
    NotifyUpgrade();
  }

 private:
  DISALLOW_COPY_AND_ASSIGN(FakeUpgradeDetector);
};

}  // namespace

// A test harness that provides facilities for manipulating the restart
// notification policy setting and for broadcasting upgrade notifications.
class RestartNotificationControllerTest : public ::testing::Test {
 protected:
  RestartNotificationControllerTest()
      : scoped_local_state_(TestingBrowserProcess::GetGlobal()) {}
  UpgradeDetector* upgrade_detector() { return &upgrade_detector_; }
  FakeUpgradeDetector& fake_upgrade_detector() { return upgrade_detector_; }

  // Sets the browser.restart_notification preference in Local State to |value|.
  void SetNotificationPref(int value) {
    scoped_local_state_.Get()->SetManagedPref(
        prefs::kRestartNotification, std::make_unique<base::Value>(value));
  }

 private:
  ScopedTestingLocalState scoped_local_state_;
  FakeUpgradeDetector upgrade_detector_;

  DISALLOW_COPY_AND_ASSIGN(RestartNotificationControllerTest);
};

TEST_F(RestartNotificationControllerTest, CreateDestroy) {
  ::testing::StrictMock<MockControllerDelegate> mock_controller_delegate;

  FakeRestartNotificationController controller(upgrade_detector(),
                                               &mock_controller_delegate);
}

// Without the browser.restart_notification preference set, the controller
// should not be observing the UpgradeDetector, and should therefore never
// attempt to show any notifications.
TEST_F(RestartNotificationControllerTest, PolicyUnset) {
  ::testing::StrictMock<MockControllerDelegate> mock_controller_delegate;

  FakeRestartNotificationController controller(upgrade_detector(),
                                               &mock_controller_delegate);

  fake_upgrade_detector().BroadcastLevelChange(
      UpgradeDetector::UPGRADE_ANNOYANCE_LOW);
  fake_upgrade_detector().BroadcastLevelChange(
      UpgradeDetector::UPGRADE_ANNOYANCE_ELEVATED);
  fake_upgrade_detector().BroadcastLevelChange(
      UpgradeDetector::UPGRADE_ANNOYANCE_HIGH);
  fake_upgrade_detector().BroadcastLevelChange(
      UpgradeDetector::UPGRADE_ANNOYANCE_SEVERE);
  fake_upgrade_detector().BroadcastLevelChange(
      UpgradeDetector::UPGRADE_ANNOYANCE_NONE);
}

// With the browser.restart_notification preference set to 1, the controller
// should be observing the UpgradeDetector and should show "Requested"
// notifications on each level change.
TEST_F(RestartNotificationControllerTest, RecommendedByPolicy) {
  SetNotificationPref(1);
  ::testing::StrictMock<MockControllerDelegate> mock_controller_delegate;

  FakeRestartNotificationController controller(upgrade_detector(),
                                               &mock_controller_delegate);

  // Nothing shown if the level is broadcast at NONE.
  fake_upgrade_detector().BroadcastLevelChange(
      UpgradeDetector::UPGRADE_ANNOYANCE_NONE);
  ::testing::Mock::VerifyAndClear(&mock_controller_delegate);

  // Show for each level change, but not for repeat notifications.
  EXPECT_CALL(mock_controller_delegate, ShowRestartRecommendedBubble());
  fake_upgrade_detector().BroadcastLevelChange(
      UpgradeDetector::UPGRADE_ANNOYANCE_LOW);
  ::testing::Mock::VerifyAndClear(&mock_controller_delegate);
  fake_upgrade_detector().BroadcastLevelChange(
      UpgradeDetector::UPGRADE_ANNOYANCE_LOW);
  ::testing::Mock::VerifyAndClear(&mock_controller_delegate);

  EXPECT_CALL(mock_controller_delegate, ShowRestartRecommendedBubble());
  fake_upgrade_detector().BroadcastLevelChange(
      UpgradeDetector::UPGRADE_ANNOYANCE_ELEVATED);
  ::testing::Mock::VerifyAndClear(&mock_controller_delegate);
  fake_upgrade_detector().BroadcastLevelChange(
      UpgradeDetector::UPGRADE_ANNOYANCE_ELEVATED);
  ::testing::Mock::VerifyAndClear(&mock_controller_delegate);

  EXPECT_CALL(mock_controller_delegate, ShowRestartRecommendedBubble());
  fake_upgrade_detector().BroadcastLevelChange(
      UpgradeDetector::UPGRADE_ANNOYANCE_HIGH);
  ::testing::Mock::VerifyAndClear(&mock_controller_delegate);
  fake_upgrade_detector().BroadcastLevelChange(
      UpgradeDetector::UPGRADE_ANNOYANCE_HIGH);
  ::testing::Mock::VerifyAndClear(&mock_controller_delegate);

  EXPECT_CALL(mock_controller_delegate, ShowRestartRecommendedBubble());
  fake_upgrade_detector().BroadcastLevelChange(
      UpgradeDetector::UPGRADE_ANNOYANCE_SEVERE);
  ::testing::Mock::VerifyAndClear(&mock_controller_delegate);
  fake_upgrade_detector().BroadcastLevelChange(
      UpgradeDetector::UPGRADE_ANNOYANCE_SEVERE);
  ::testing::Mock::VerifyAndClear(&mock_controller_delegate);

  EXPECT_CALL(mock_controller_delegate, ShowRestartRecommendedBubble());
  fake_upgrade_detector().BroadcastLevelChange(
      UpgradeDetector::UPGRADE_ANNOYANCE_HIGH);
  ::testing::Mock::VerifyAndClear(&mock_controller_delegate);
  fake_upgrade_detector().BroadcastLevelChange(
      UpgradeDetector::UPGRADE_ANNOYANCE_HIGH);
  ::testing::Mock::VerifyAndClear(&mock_controller_delegate);

  // And closed if the level drops back to none.
  EXPECT_CALL(mock_controller_delegate, CloseRestartRecommendedBubble());
  fake_upgrade_detector().BroadcastLevelChange(
      UpgradeDetector::UPGRADE_ANNOYANCE_NONE);
  ::testing::Mock::VerifyAndClear(&mock_controller_delegate);
  fake_upgrade_detector().BroadcastLevelChange(
      UpgradeDetector::UPGRADE_ANNOYANCE_NONE);
  ::testing::Mock::VerifyAndClear(&mock_controller_delegate);
}

// With the browser.restart_notification preference set to 2, the controller
// should be observing the UpgradeDetector and should show "Required"
// notifications on each level change.
TEST_F(RestartNotificationControllerTest, RequiredByPolicy) {
  SetNotificationPref(2);
  ::testing::StrictMock<MockControllerDelegate> mock_controller_delegate;

  FakeRestartNotificationController controller(upgrade_detector(),
                                               &mock_controller_delegate);

  // Nothing shown if the level is broadcast at NONE.
  fake_upgrade_detector().BroadcastLevelChange(
      UpgradeDetector::UPGRADE_ANNOYANCE_NONE);
  ::testing::Mock::VerifyAndClear(&mock_controller_delegate);

  // Show for each level change, but not for repeat notifications.
  EXPECT_CALL(mock_controller_delegate, ShowRestartRequiredDialog());
  fake_upgrade_detector().BroadcastLevelChange(
      UpgradeDetector::UPGRADE_ANNOYANCE_LOW);
  ::testing::Mock::VerifyAndClear(&mock_controller_delegate);
  fake_upgrade_detector().BroadcastLevelChange(
      UpgradeDetector::UPGRADE_ANNOYANCE_LOW);
  ::testing::Mock::VerifyAndClear(&mock_controller_delegate);

  EXPECT_CALL(mock_controller_delegate, ShowRestartRequiredDialog());
  fake_upgrade_detector().BroadcastLevelChange(
      UpgradeDetector::UPGRADE_ANNOYANCE_ELEVATED);
  ::testing::Mock::VerifyAndClear(&mock_controller_delegate);
  fake_upgrade_detector().BroadcastLevelChange(
      UpgradeDetector::UPGRADE_ANNOYANCE_ELEVATED);
  ::testing::Mock::VerifyAndClear(&mock_controller_delegate);

  EXPECT_CALL(mock_controller_delegate, ShowRestartRequiredDialog());
  fake_upgrade_detector().BroadcastLevelChange(
      UpgradeDetector::UPGRADE_ANNOYANCE_HIGH);
  ::testing::Mock::VerifyAndClear(&mock_controller_delegate);
  fake_upgrade_detector().BroadcastLevelChange(
      UpgradeDetector::UPGRADE_ANNOYANCE_HIGH);
  ::testing::Mock::VerifyAndClear(&mock_controller_delegate);

  EXPECT_CALL(mock_controller_delegate, ShowRestartRequiredDialog());
  fake_upgrade_detector().BroadcastLevelChange(
      UpgradeDetector::UPGRADE_ANNOYANCE_SEVERE);
  ::testing::Mock::VerifyAndClear(&mock_controller_delegate);
  fake_upgrade_detector().BroadcastLevelChange(
      UpgradeDetector::UPGRADE_ANNOYANCE_SEVERE);
  ::testing::Mock::VerifyAndClear(&mock_controller_delegate);

  EXPECT_CALL(mock_controller_delegate, ShowRestartRequiredDialog());
  fake_upgrade_detector().BroadcastLevelChange(
      UpgradeDetector::UPGRADE_ANNOYANCE_HIGH);
  ::testing::Mock::VerifyAndClear(&mock_controller_delegate);
  fake_upgrade_detector().BroadcastLevelChange(
      UpgradeDetector::UPGRADE_ANNOYANCE_HIGH);
  ::testing::Mock::VerifyAndClear(&mock_controller_delegate);

  // And closed if the level drops back to none.
  EXPECT_CALL(mock_controller_delegate, CloseRestartRequiredDialog());
  fake_upgrade_detector().BroadcastLevelChange(
      UpgradeDetector::UPGRADE_ANNOYANCE_NONE);
  ::testing::Mock::VerifyAndClear(&mock_controller_delegate);
  fake_upgrade_detector().BroadcastLevelChange(
      UpgradeDetector::UPGRADE_ANNOYANCE_NONE);
  ::testing::Mock::VerifyAndClear(&mock_controller_delegate);
}

// Flipping the policy should have no effect when at level NONE
TEST_F(RestartNotificationControllerTest, PolicyChangesNoUpgrade) {
  ::testing::StrictMock<MockControllerDelegate> mock_controller_delegate;

  FakeRestartNotificationController controller(upgrade_detector(),
                                               &mock_controller_delegate);

  SetNotificationPref(1);
  ::testing::Mock::VerifyAndClear(&mock_controller_delegate);

  SetNotificationPref(2);
  ::testing::Mock::VerifyAndClear(&mock_controller_delegate);

  SetNotificationPref(3);  // Bogus value!
  ::testing::Mock::VerifyAndClear(&mock_controller_delegate);

  SetNotificationPref(0);
  ::testing::Mock::VerifyAndClear(&mock_controller_delegate);
}

// Policy changes at an elevated level should show the appropriate notification.
TEST_F(RestartNotificationControllerTest, PolicyChangesWithUpgrade) {
  ::testing::StrictMock<MockControllerDelegate> mock_controller_delegate;

  FakeRestartNotificationController controller(upgrade_detector(),
                                               &mock_controller_delegate);

  fake_upgrade_detector().BroadcastLevelChange(
      UpgradeDetector::UPGRADE_ANNOYANCE_LOW);
  ::testing::Mock::VerifyAndClear(&mock_controller_delegate);

  EXPECT_CALL(mock_controller_delegate, ShowRestartRecommendedBubble());
  SetNotificationPref(1);
  ::testing::Mock::VerifyAndClear(&mock_controller_delegate);

  EXPECT_CALL(mock_controller_delegate, CloseRestartRecommendedBubble());
  EXPECT_CALL(mock_controller_delegate, ShowRestartRequiredDialog());
  SetNotificationPref(2);
  ::testing::Mock::VerifyAndClear(&mock_controller_delegate);

  EXPECT_CALL(mock_controller_delegate, CloseRestartRequiredDialog());
  SetNotificationPref(0);
  ::testing::Mock::VerifyAndClear(&mock_controller_delegate);
}
