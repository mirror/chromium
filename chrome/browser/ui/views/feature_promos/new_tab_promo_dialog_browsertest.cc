// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/feature_engagement/new_tab/new_tab_tracker.h"
#include "chrome/browser/feature_engagement/new_tab/new_tab_tracker_factory.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/test/test_browser_dialog.h"

class NewTabPromoDialogTest : public DialogBrowserTest {
 public:
  NewTabPromoDialogTest() = default;

  // DialogBrowserTest:
  void ShowDialog(const std::string& name) override {
    feature_engagement::NewTabTrackerFactory::GetInstance()
        ->GetForProfile(browser()->profile())
        ->ShowPromo();
  }

 private:
  DISALLOW_COPY_AND_ASSIGN(NewTabPromoDialogTest);
};

// Test that calls ShowDialog("default"). Interactive when run via
// ../browser_tests --gtest_filter=BrowserDialogTest.Invoke
// --interactive --dialog=NewTabPromoDialogTest.InvokeDialog_NewTabPromo
IN_PROC_BROWSER_TEST_F(NewTabPromoDialogTest, InvokeDialog_NewTabPromo) {
  RunDialog();
}
