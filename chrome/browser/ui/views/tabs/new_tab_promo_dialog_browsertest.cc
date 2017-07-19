// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/test/test_browser_dialog.h"
#include "chrome/browser/ui/views/frame/browser_view.h"
#include "chrome/browser/ui/views/tabs/new_tab_button.h"
#include "chrome/browser/ui/views/tabs/tab_strip.h"
#include "chrome/grit/generated_resources.h"
#include "ui/base/l10n/l10n_util.h"

class NewTabPromoDialogTest : public DialogBrowserTest,
                              public testing::WithParamInterface<const char*> {
 public:
  NewTabPromoDialogTest() = default;

  // DialogBrowserTest:
  void ShowDialog(const std::string& name) override {
    BrowserView* browser_view =
        static_cast<BrowserView*>(InProcessBrowserTest::browser()->window());

    browser_view->tabstrip()->new_tab_button()->ShowPromo(GetParam());
  }

 private:
  DISALLOW_COPY_AND_ASSIGN(NewTabPromoDialogTest);
};

// The following test is for the three potential strings being considered
// for the NewTabPromo.
//
// Test that calls ShowDialog("default"). Interactive when run via
// out\Default\browser_tests --gtest_filter=BrowserDialogTest.Invoke
// --interactive --dialog=PromoStrings/NewTabPromoDialogTest.
// InvokeDialog_NewTabPromo/N
//
// Where N is one of the following {1,2,3} which corrispond with
// {"IDS_NEWTAB_PROMO_1", "IDS_NEWTAB_PROMO_2", "IDS_NEWTAB_PROMO_3"}.

IN_PROC_BROWSER_TEST_P(NewTabPromoDialogTest, InvokeDialog_NewTabPromo) {
  RunDialog();
}

INSTANTIATE_TEST_CASE_P(PromoStrings,
                        NewTabPromoDialogTest,
                        testing::Values("IDS_NEWTAB_PROMO_1",
                                        "IDS_NEWTAB_PROMO_2",
                                        "IDS_NEWTAB_PROMO_3"));
