// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_TEST_TEST_BROWSER_DIALOG_H_
#define CHROME_BROWSER_UI_TEST_TEST_BROWSER_DIALOG_H_

#include "base/macros.h"
#include "chrome/browser/ui/test/test_browser_ui.h"
#include "chrome/test/base/in_process_browser_test.h"

#if defined(TOOLKIT_VIEWS)
#include "ui/views/widget/widget.h"
#endif

// A dialog-specific subclass of TestBrowserUI, which will verify that a test
// showed a single dialog.
class TestBrowserDialog : public TestBrowserUI {
 protected:
  TestBrowserDialog();
  ~TestBrowserDialog() override;

  // TestBrowserUI:
  void PreShow() override;
  bool VerifyUI() override;
  void WaitForUserDismissal() override;
  void DismissUI() override;

  // Whether to close asynchronously using Widget::Close(). This covers
  // codepaths relying on DialogDelegate::Close(), which isn't invoked by
  // Widget::CloseNow(). Dialogs should support both, since the OS can initiate
  // the destruction of dialogs, e.g., during logoff which bypass
  // Widget::CanClose() and DialogDelegate::Close().
  virtual bool AlwaysCloseAsynchronously();

 private:
#if defined(TOOLKIT_VIEWS)
  // Stores the current widgets in |widgets_|.
  void UpdateWidgets();

  // The widgets present before/after showing UI.
  views::Widget::Widgets widgets_;
#endif  // defined(TOOLKIT_VIEWS)

  DISALLOW_COPY_AND_ASSIGN(TestBrowserDialog);
};

template <class Base>
using SupportsTestDialog = SupportsTestUI<Base, TestBrowserDialog>;

using DialogBrowserTest = SupportsTestDialog<InProcessBrowserTest>;

#endif  // CHROME_BROWSER_UI_TEST_TEST_BROWSER_DIALOG_H_
