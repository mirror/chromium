// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/chromeos/arc/accessibility/arc_accessibility_helper_bridge.h"

#include "ash/public/cpp/accessibility_types.h"
#include "chrome/browser/chromeos/accessibility/accessibility_manager.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "components/arc/arc_bridge_service.h"
#include "components/arc/arc_service_manager.h"
#include "components/arc/arc_util.h"
#include "components/arc/test/connection_holder_util.h"
#include "components/arc/test/fake_accessibility_helper_instance.h"

namespace arc {

class ArcAccessibilityHelperBridgeBrowserTest : public InProcessBrowserTest {
  void SetUpCommandLine(base::CommandLine* command_line) override {
    arc::SetArcAvailableCommandLineForTesting(command_line);
  }

  void SetUpOnMainThread() override {
    chromeos::AccessibilityManager::Get()->SetProfileForTest(
        browser()->profile());
  }

  void TearDownOnMainThread() override {
    if (fake_accessibility_helper_instance_) {
      ArcServiceManager::Get()
          ->arc_bridge_service()
          ->accessibility_helper()
          ->CloseInstance(fake_accessibility_helper_instance_.get());
      fake_accessibility_helper_instance_.reset();
    }
  }

 protected:
  void SetFakeInstance() {
    fake_accessibility_helper_instance_ =
        std::make_unique<FakeAccessibilityHelperInstance>();
    ArcServiceManager::Get()
        ->arc_bridge_service()
        ->accessibility_helper()
        ->SetInstance(fake_accessibility_helper_instance_.get());
    WaitForInstanceReady(
        ArcServiceManager::Get()->arc_bridge_service()->accessibility_helper());
  }

  std::unique_ptr<FakeAccessibilityHelperInstance>
      fake_accessibility_helper_instance_;
};

IN_PROC_BROWSER_TEST_F(ArcAccessibilityHelperBridgeBrowserTest,
                       UpdateFilterType) {
  // When accessibility preference is changed, Android side create an instance
  // by it. This is the case where an instance is created by focus highlight.
  chromeos::AccessibilityManager::Get()->SetFocusHighlightEnabled(true);
  SetFakeInstance();
  ASSERT_EQ(mojom::AccessibilityFilterType::FOCUS,
            fake_accessibility_helper_instance_->filter_type());

  chromeos::AccessibilityManager::Get()->EnableSpokenFeedback(
      true /* enable */, ash::A11Y_NOTIFICATION_NONE);
  EXPECT_EQ(mojom::AccessibilityFilterType::WHITELISTED_PACKAGE_NAME,
            fake_accessibility_helper_instance_->filter_type());
}

}  // namespace arc
