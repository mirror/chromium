// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_TEST_TEST_BROWSER_DIALOG_H_
#define CHROME_BROWSER_UI_TEST_TEST_BROWSER_DIALOG_H_

#include <string>

#include "base/macros.h"
#include "base/test/scoped_feature_list.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "ui/gfx/native_widget_types.h"

// TestBrowserUI provides a way to register an InProcessBrowserTest testing
// harness with a framework that invokes Chrome browser UI in a consistent way.
// It optionally provides a way to invoke dialogs "interactively". This allows
// screenshots to be generated easily, with the same test data, to assist with
// UI review. It also provides a UI registry so pieces of UI can be
// systematically checked for subtle changes and regressions.
//
// To use TestBrowserUI, a test harness should inherit from BrowserUITest rather
// than InProcessBrowserTest. If the UI under test has only a single mode of
// operation, the only other requirement on the test harness is an override:
//
// class FooUITest : public BrowserUITest {
//  public:
//   ..
//   // BrowserUITest:
//   void ShowUI(const std::string& name) override {
//     /* Show UI attached to browser() and leave it open. */
//   }
//   ..
// };
//
// The test may then define any number of cases for individual pieces of UI:
//
// IN_PROC_BROWSER_TEST_F(FooUITest, InvokeUI_name) {
//   // Perform optional setup here; then:
//   ShowAndVerifyUI();
// }
//
// The string after "InvokeUI_" (here, "name") is the argument given to
// ShowUI(). In a regular test suite run, ShowAndVerifyUI() shows the UI and
// immediately closes it (after ensuring it was actually created).
//
// To get a list of all available UI, run the "BrowserUITest.Invoke" test case
// without other arguments, i.e.:
//
//   browser_tests --gtest_filter=BrowserUITest.Invoke
//
// UI listed can be shown interactively using the --ui argument. E.g.
//
//   browser_tests --gtest_filter=BrowserUITest.Invoke --interactive
//       --ui=FooUITest.InvokeUI_name
template <class Base>
class TestBrowserUI : public InProcessBrowserTest {
 protected:
  TestBrowserUI() = default;

  // Called by ShowAndVerifyUI() before ShowUI(), to provide a place to do any
  // setup needed in order to successfully verify the UI post-show.
  virtual void PreShow() {}

  // Should be implemented in individual tests to show UI with the given |name|
  // (which will be supplied by the test case).
  virtual void ShowUI(const std::string& name) = 0;

  // Called by ShowAndVerifyUI() after ShowUI().  Returns whether the UI was
  // successfully shown.
  virtual bool VerifyUI() = 0;

  // Called by ShowAndVerifyUI() after VerifyUI(), in the case where the test is
  // interactive.  This should block until the UI has been dismissed.
  virtual void WaitForUserDismissal() = 0;

  // Called by ShowAndVerifyUI() after VerifyUI(), in the case where the test is
  // non-interactive.  This should do anything necessary to close the UI before
  // browser shutdown.
  virtual void DismissUI() {}

  // Shows the UI whose name corresponds to the current test case, and verifies
  // it was successfully shown.  Most test cases can simply invoke this directly
  // with no other code.
  void ShowAndVerifyUI();

  // Convenience method to force-enable features::kSecondaryUiMd for this test
  // on all platforms. This should be called in an override of SetUp().
  void UseMdOnly();

 private:
  base::test::ScopedFeatureList maybe_enable_md_;

  DISALLOW_COPY_AND_ASSIGN(TestBrowserUI);
};

// A dialog-specific subclass of TestBrowserUI, which will verify that a test
// showed a single dialog.
class TestBrowserDialog : public TestBrowserUI {
 protected:
  TestBrowserDialog() = default;

  // TestBrowserUI:
  void PreShow() override;
  void VerifyUI() override;
  void DismissUI() override;

  // Whether to close asynchronously using Widget::CloseNow(). This covers
  // codepaths relying on DialogDelegate::Close(), which isn't invoked by
  // Widget::CloseNow(). Dialogs should support both, since the OS can initiate
  // the destruction of dialogs, e.g., during logoff which bypass
  // Widget::CanClose() and DialogDelegate::Close().
  virtual bool AlwaysCloseAsynchronously();

 private:
  // Stores the current widgets in |widgets_|.
  void GetAllWidgets();

  // The widgets present before/after showing UI.
  views::Widget::Widgets widgets_;

  DISALLOW_COPY_AND_ASSIGN(TestBrowserDialog);
};

namespace internal {

// When present on the command line, runs the test in an interactive mode.
constexpr const char kInteractiveSwitch[] = "interactive";

}  // namespace internal

#endif  // CHROME_BROWSER_UI_TEST_TEST_BROWSER_DIALOG_H_
