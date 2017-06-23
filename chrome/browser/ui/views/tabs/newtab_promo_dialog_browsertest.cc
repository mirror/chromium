// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/command_line.h"
#include "chrome/browser/ui/browser_window.h"
#include "chrome/browser/ui/test/test_browser_dialog.h"
#include "chrome/browser/ui/views/tabs/tab_strip.h"
#include "ui/base/ui_base_switches.h"

class NewTabPromoDialogTest : public DialogBrowserTest {
 public:
  NewTabPromoDialogTest() {}

  // DialogBrowserTest:
  void ShowDialog(const std::string& name) override {
    InProcessBrowserTest::browser()->tabstrip()->ShowPromo();
  }

  // content::BrowserTestBase:
  void SetUpCommandLine(base::CommandLine* command_line) override {
    command_line->AppendSwitch(switches::kExtendMdToSecondaryUi);
  }

 private:
  DISALLOW_COPY_AND_ASSIGN(NewTabPromoDialogTest);
};

// Test that calls ShowDialog("default"). Interactive when run via
// browser_tests --gtest_filter=BrowserDialogTest.Invoke --interactive
// --dialog=UpdateRecommendedDialogTest.InvokeDialog_default
IN_PROC_BROWSER_TEST_F(NewTabPromoDialogTest, InvokeDialog_NewTabPromo) {
  RunDialog();
}
