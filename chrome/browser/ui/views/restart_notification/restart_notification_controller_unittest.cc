// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/views/restart_notification/restart_notification_controller.h"

#include "base/macros.h"
#include "chrome/browser/upgrade_detector.h"
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

 protected:
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

 private:
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

 private:
  DISALLOW_COPY_AND_ASSIGN(FakeUpgradeDetector);
};

}  // namespace

class RestartNotificationControllerTest : public ::testing::Test {
 protected:
  RestartNotificationControllerTest()
      : scoped_local_state_(TestingBrowserProcess::GetGlobal()) {}
  UpgradeDetector* upgrade_detector() { return &upgrade_detector_; }

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
