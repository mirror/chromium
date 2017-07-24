// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/test/test_browser_dialog.h"
#include "chrome/browser/ui/views/frame/browser_view.h"
#include "chrome/browser/ui/views/toolbar/toolbar_view.h"

class BookmarkPromoBubbleViewDialogTest : public DialogBrowserTest {
 public:
  BookmarkPromoBubbleViewDialogTest() = default;

  // DialogBrowserTest:
  void ShowDialog(const std::string& name) override {
    BrowserView* browser_view =
        BrowserView::GetBrowserViewForBrowser(browser());
    browser_view->toolbar()->ShowBookmarkPromoBubble();
  }

 private:
  DISALLOW_COPY_AND_ASSIGN(BookmarkPromoBubbleViewDialogTest);
};

// Test that calls ShowDialog("default"). Interactive when run via
// browser_tests --gtest_filter=BrowserDialogTest.Invoke --interactive
// --dialog=BookmarkPromoBubbleViewDialogTest.InvokeDialog_BookmarkPromoBubble
IN_PROC_BROWSER_TEST_F(BookmarkPromoBubbleViewDialogTest,
                       InvokeDialog_BookmarkPromoBubble) {
  RunDialog();
}
