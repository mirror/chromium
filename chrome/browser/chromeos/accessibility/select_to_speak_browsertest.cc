// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <memory>
#include <vector>

#include "ash/accessibility/accessibility_focus_ring_controller.h"
#include "ash/accessibility/accessibility_focus_ring_layer.h"
#include "ash/shell.h"
#include "ash/system/tray/system_tray.h"
#include "base/bind.h"
#include "base/strings/pattern.h"
#include "chrome/browser/chromeos/accessibility/accessibility_manager.h"
#include "chrome/browser/chromeos/accessibility/speech_monitor.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_window.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/ui_test_utils.h"
#include "content/public/browser/notification_details.h"
#include "content/public/browser/notification_service.h"
#include "extensions/browser/notification_types.h"
#include "ui/events/test/event_generator.h"
#include "url/url_constants.h"

namespace chromeos {

class SelectToSpeakTest : public InProcessBrowserTest {
 public:
  void OnFocusRingChanged() {
    if (loop_runner_) {
      loop_runner_->Quit();
    }
  }

 protected:
  SelectToSpeakTest() {}
  ~SelectToSpeakTest() override {}

  void SetUpOnMainThread() override {
    ASSERT_FALSE(AccessibilityManager::Get()->IsSelectToSpeakEnabled());

    content::WindowedNotificationObserver extension_load_waiter(
        extensions::NOTIFICATION_EXTENSION_HOST_DID_STOP_FIRST_LOAD,
        content::NotificationService::AllSources());
    AccessibilityManager::Get()->SetSelectToSpeakEnabled(true);
    extension_load_waiter.Wait();

    aura::Window* root_window = ash::Shell::Get()->GetPrimaryRootWindow();
    generator_.reset(new ui::test::EventGenerator(root_window));

    ui_test_utils::NavigateToURL(browser(), GURL(url::kAboutBlankURL));
  }

  SpeechMonitor speech_monitor_;
  std::unique_ptr<ui::test::EventGenerator> generator_;

  void ActivateSelectToSpeakInWindowBounds(std::string url) {
    ui_test_utils::NavigateToURL(browser(), GURL(url));
    gfx::Rect bounds = browser()->window()->GetBounds();

    // Hold down Search and click a few pixels into the window bounds.
    generator_->PressKey(ui::VKEY_LWIN, 0 /* flags */);
    generator_->MoveMouseTo(bounds.x() + 8, bounds.y() + 50);
    generator_->PressLeftButton();
    generator_->MoveMouseTo(bounds.x() + bounds.width() - 8,
                            bounds.y() + bounds.height() - 8);
    generator_->ReleaseLeftButton();
    generator_->ReleaseKey(ui::VKEY_LWIN, 0 /* flags */);
  }

  // Blocks until the focus ring is changed.
  void WaitForFocusRingChanged() {
    loop_runner_ = new content::MessageLoopRunner();
    loop_runner_->Run();
    loop_runner_ = NULL;
  }

 private:
  scoped_refptr<content::MessageLoopRunner> loop_runner_;
  DISALLOW_COPY_AND_ASSIGN(SelectToSpeakTest);
};

IN_PROC_BROWSER_TEST_F(SelectToSpeakTest, SpeakStatusTray) {
  ash::SystemTray* tray = ash::Shell::Get()->GetPrimarySystemTray();
  gfx::Rect tray_bounds = tray->GetBoundsInScreen();

  // Hold down Search and click a few pixels into the status tray bounds.
  generator_->PressKey(ui::VKEY_LWIN, 0 /* flags */);
  generator_->MoveMouseTo(tray_bounds.x() + 8, tray_bounds.y() + 8);
  generator_->PressLeftButton();
  generator_->ReleaseLeftButton();
  generator_->ReleaseKey(ui::VKEY_LWIN, 0 /* flags */);

  EXPECT_TRUE(
      base::MatchPattern(speech_monitor_.GetNextUtterance(), "Status tray*"));
}

IN_PROC_BROWSER_TEST_F(SelectToSpeakTest, SearchLocksToSelectToSpeakMode) {
  ui_test_utils::NavigateToURL(browser(), GURL("data:text/html;charset=utf-8,"
                                               "<p>This is some text</p>"));
  gfx::Rect bounds = browser()->window()->GetBounds();

  // Hold click Search, then and click a few pixels into the window bounds.
  generator_->PressKey(ui::VKEY_LWIN, 0 /* flags */);
  generator_->ReleaseKey(ui::VKEY_LWIN, 0 /* flags */);

  for (int i = 0; i < 2; ++i) {
    // With the mouse only, have it speak a few times.
    generator_->MoveMouseTo(bounds.x() + 8, bounds.y() + 50);
    generator_->PressLeftButton();
    generator_->MoveMouseTo(bounds.x() + bounds.width() - 8,
                            bounds.y() + bounds.height() - 8);
    generator_->ReleaseLeftButton();

    EXPECT_TRUE(base::MatchPattern(speech_monitor_.GetNextUtterance(),
                                   "This is some text*"));
  }
}

IN_PROC_BROWSER_TEST_F(SelectToSpeakTest, SmoothlyReadsAcrossInlineUrl) {
  // Make sure an inline URL is read smoothly.
  ActivateSelectToSpeakInWindowBounds(
      "data:text/html;charset=utf-8,<p>This is some text <a href=>with a"
      " node</a> in the middle");
  // Should combine nodes in a paragraph into one utterance.
  // Includes some wildcards between words because there may be extra
  // spaces. Spaces are not pronounced, so extra spaces do not impact output.
  EXPECT_TRUE(
      base::MatchPattern(speech_monitor_.GetNextUtterance(),
                         "This is some text*with a node*in the middle*"));
}

IN_PROC_BROWSER_TEST_F(SelectToSpeakTest, SmoothlyReadsAcrossMultipleLines) {
  // Sentences spanning multiple lines.
  ActivateSelectToSpeakInWindowBounds(
      "data:text/html;charset=utf-8,<div style=\"width:100px\">This"
      " is some text with a node in the middle");
  // Should combine nodes in a paragraph into one utterance.
  // Includes some wildcards between words because there may be extra
  // spaces, for example at line wraps. Extra wildcards included to
  // reduce flakyness in case wrapping is not consistent.
  // Spaces are not pronounced, so extra spaces do not impact output.
  EXPECT_TRUE(
      base::MatchPattern(speech_monitor_.GetNextUtterance(),
                         "This is some*text*with*a*node*in*the*middle*"));
}

IN_PROC_BROWSER_TEST_F(SelectToSpeakTest, SmoothlyReadsAcrossFormattedText) {
  // Bold or formatted text
  ActivateSelectToSpeakInWindowBounds(
      "data:text/html;charset=utf-8,<p>This is some text <b>with a node"
      "</b> in the middle");

  // Should combine nodes in a paragraph into one utterance.
  // Includes some wildcards between words because there may be extra
  // spaces. Spaces are not pronounced, so extra spaces do not impact output.
  EXPECT_TRUE(
      base::MatchPattern(speech_monitor_.GetNextUtterance(),
                         "This is some text*with a node*in the middle*"));
}

IN_PROC_BROWSER_TEST_F(SelectToSpeakTest, BreaksAtParagraphBounds) {
  ActivateSelectToSpeakInWindowBounds(
      "data:text/html;charset=utf-8,<div><p>First paragraph</p>"
      "<p>Second paragraph</p></div>");

  // Should keep each paragraph as its own utterance.
  EXPECT_TRUE(base::MatchPattern(speech_monitor_.GetNextUtterance(),
                                 "First paragraph*"));
  EXPECT_TRUE(base::MatchPattern(speech_monitor_.GetNextUtterance(),
                                 "Second paragraph*"));
}

IN_PROC_BROWSER_TEST_F(SelectToSpeakTest, FocusRingMovesWithMouse) {
  ash::AccessibilityFocusRingController* controller =
      ash::AccessibilityFocusRingController::GetInstance();
  // Create a callback for the focus ring observer.
  base::RepeatingCallback<void()> callback = base::BindRepeating(
      &SelectToSpeakTest::OnFocusRingChanged, base::Unretained(this));
  controller->SetFocusRingObserver(callback);

  std::vector<std::unique_ptr<ash::AccessibilityFocusRingLayer>>* focus_rings =
      controller->focus_ring_layers_for_testing();

  // No focus rings to start.
  EXPECT_EQ((int)focus_rings->size(), 0);

  ui_test_utils::NavigateToURL(browser(), GURL("data:text/html;charset=utf-8,"
                                               "<p>This is some text</p>"));
  gfx::Rect bounds = browser()->window()->GetBounds();
  generator_->PressKey(ui::VKEY_LWIN, 0 /* flags */);
  generator_->MoveMouseTo(bounds.x() + 8, bounds.y() + 50);
  generator_->PressLeftButton();

  // Expect a focus ring to have been drawn.
  WaitForFocusRingChanged();
  EXPECT_EQ((int)focus_rings->size(), 1);

  // Make sure it's in a reasonable position.
  EXPECT_LT(abs(focus_rings->at(0)->layer()->GetTargetBounds().x() -
                (bounds.x() + 8)),
            50);
  EXPECT_LT(abs(focus_rings->at(0)->layer()->GetTargetBounds().y() -
                (bounds.y() + 50)),
            50);
  EXPECT_LT(focus_rings->at(0)->layer()->GetTargetBounds().width(), 50);
  EXPECT_LT(focus_rings->at(0)->layer()->GetTargetBounds().height(), 50);

  // Move the mouse.
  generator_->MoveMouseTo(bounds.x() + 108, bounds.y() + 158);

  // Expect focus ring to have moved with the mouse.
  // The size should have grown to be over 100 (the rect is now size 100,
  // and the focus ring has some buffer). Position should be unchanged.
  WaitForFocusRingChanged();
  EXPECT_LT(abs(focus_rings->at(0)->layer()->GetTargetBounds().x() -
                (bounds.x() + 8)),
            50);
  EXPECT_LT(abs(focus_rings->at(0)->layer()->GetTargetBounds().y() -
                (bounds.y() + 50)),
            50);
  EXPECT_GT(focus_rings->at(0)->layer()->GetTargetBounds().width(), 100);
  EXPECT_GT(focus_rings->at(0)->layer()->GetTargetBounds().height(), 100);

  // Move the mouse smaller again, it should shrink.
  generator_->MoveMouseTo(bounds.x() + 18, bounds.y() + 68);
  WaitForFocusRingChanged();
  EXPECT_LT(focus_rings->at(0)->layer()->GetTargetBounds().width(), 50);
  EXPECT_LT(focus_rings->at(0)->layer()->GetTargetBounds().height(), 50);

  // Cancel this by releasing the key before the mouse.
  generator_->ReleaseKey(ui::VKEY_LWIN, 0 /* flags */);
  generator_->ReleaseLeftButton();

  // Expect focus ring to have been cleared, this was canceled in STS
  // by releasing the key before the button.
  WaitForFocusRingChanged();
  EXPECT_EQ((int)focus_rings->size(), 0);

  // Clean up
  controller->ClearFocusRingObserver();
}

}  // namespace chromeos
