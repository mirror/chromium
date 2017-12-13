// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_TEST_TEST_BROWSER_UI_H_
#define CHROME_BROWSER_UI_TEST_TEST_BROWSER_UI_H_

#include <memory>
#include <string>

#include "base/macros.h"
#include "chrome/test/base/in_process_browser_test.h"

namespace base {
namespace test {
class ScopedFeatureList;
}  // namespace test
}  // namespace base

// TestBrowserUI provides a way to register an InProcessBrowserTest testing
// harness with a framework that invokes Chrome browser UI in a consistent way.
// It optionally provides a way to invoke UI "interactively". This allows
// screenshots to be generated easily, with the same test data, to assist with
// UI review. It also provides a UI registry so pieces of UI can be
// systematically checked for subtle changes and regressions.
//
// To use TestBrowserUI, a test harness should inherit from UIBrowserTest rather
// than InProcessBrowserTest, then provide some overrides:
//
// class FooUITest : public UIBrowserTest {
//  public:
//   ..
//   // UIBrowserTest:
//   void ShowUI(const std::string& name) override {
//     /* Show UI attached to browser() and leave it open. */
//   }
//
//   bool VerifyUI() override {
//     /* Return true if the UI was successfully shown. */
//   }
//
//   void WaitForUserDismissal() override {
//     /* Block until the user closes the UI. */
//   }
//   ..
// };
//
// Further overrides are available for tests which need to do work before
// showing any UI or when closing in non-interactive mode.  For tests whose UI
// is a dialog, there's also the TestBrowserDialog class, which provides all but
// ShowUI() already; see test_browser_dialog.h.
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
class TestBrowserUI {
 protected:
  TestBrowserUI();
  virtual ~TestBrowserUI();

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
  // If non-null, forces secondary UI to MD.
  std::unique_ptr<base::test::ScopedFeatureList> enable_md_;

  DISALLOW_COPY_AND_ASSIGN(TestBrowserUI);
};

// Helper to mix in a TestBrowserUI to an existing test harness. |Base| must be
// a descendant of InProcessBrowserTest.
template <class Base, class TestUI>
class SupportsTestUI : public Base, public TestUI {
 protected:
  template <class... Args>
  explicit SupportsTestUI(Args&&... args) : Base(std::forward<Args>(args)...) {}

 private:
  DISALLOW_COPY_AND_ASSIGN(SupportsTestUI);
};

using UIBrowserTest = SupportsTestUI<InProcessBrowserTest, TestBrowserUI>;

namespace internal {

// When present on the command line, runs the test in an interactive mode.
constexpr const char kInteractiveSwitch[] = "interactive";

}  // namespace internal

#endif  // CHROME_BROWSER_UI_TEST_TEST_BROWSER_UI_H_
