// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/test/test_browser_dialog.h"

#include "base/command_line.h"
#include "base/run_loop.h"
#include "base/stl_util.h"
#include "base/test/gtest_util.h"
#include "base/test/scoped_feature_list.h"
#include "base/threading/thread_task_runner_handle.h"
#include "build/build_config.h"
#include "chrome/browser/platform_util.h"
#include "chrome/common/chrome_features.h"
#include "ui/base/ui_base_features.h"
#include "ui/views/test/widget_test.h"
#include "ui/views/widget/widget.h"
#include "ui/views/widget/widget_observer.h"

#if defined(OS_CHROMEOS)
#include "ash/public/cpp/config.h"
#include "ash/shell.h"  // mash-ok
#include "chrome/browser/chromeos/ash_config.h"
#endif

#if defined(OS_MACOSX)
#include "chrome/browser/ui/test/test_browser_dialog_mac.h"
#endif

namespace {

// Helper to return when a Widget has been closed.
class WidgetObserver : public views::WidgetObserver {
 public:
  explicit WidgetCloser(views::Widget* widget) : widget_(widget) {
    widget->AddObserver(this);

#if defined(OS_MACOSX)
    // TODO(pkasting): Do we only need this for the interactive case, not the
    // non-interactive case?
    internal::TestBrowserDialogInteractiveSetUp();
#endif
  }

  // views::WidgetObserver:
  void OnWidgetDestroyed(views::Widget* widget) override {
    widget_->RemoveObserver(this);
    widget_ = nullptr;
    run_loop_.Quit();
  }

  void WaitForClose() { run_loop_.Run(); }

 protected:
  views::Widget* widget() { return widget_; }

 private:
  views::Widget* widget_;
  base::RunLoop run_loop_;

  DISALLOW_COPY_AND_ASSIGN(WidgetObserver);
};

// Helper to close a Widget.  Inherits from WidgetObserver since regardless of
// whether the close is done syncronously, we always want callers to wait for it
// to complete.
class WidgetCloser : public WidgetObserver {
 public:
  WidgetCloser(views::Widget* widget, bool async) : WidgetObserver(widget) {
    base::ThreadTaskRunnerHandle::Get()->PostTask(
        FROM_HERE, base::BindOnce(&WidgetCloser::CloseWidget,
                                  weak_ptr_factory_.GetWeakPtr(), async));
  }

 private:
  void CloseWidget(bool async) {
    if (!widget())
      return;

    if (async)
      widget()->Close();
    else
      widget()->CloseNow();
  }

  base::WeakPtrFactory<WidgetCloser> weak_ptr_factory_{this};

  DISALLOW_COPY_AND_ASSIGN(WidgetCloser);
};

// Extracts the |name| argument for ShowUI() from the current test case name.
// E.g. for InvokeUI_name (or DISABLED_InvokeUI_name) returns "name".
std::string NameFromTestCase() {
  const std::string name = base::TestNameWithoutDisabledPrefix(
      testing::UnitTest::GetInstance()->current_test_info()->name());
  size_t underscore = name.find('_');
  return underscore == std::string::npos ? std::string()
                                         : name.substr(underscore + 1);
}

}  // namespace

TestBrowserUI::ShowAndVerifyUI() {
  PreShow();
  ShowUI(NameFromTestCase());
  ASSERT_TRUE(VerifyUIPostShow());
  if (base::CommandLine::ForCurrentProcess()->HasSwitch(
          internal::kInteractiveSwitch))
    WaitForUserDismissal();
  else
    DismissUI();
}

void TestBrowserUI::UseMdOnly() {
#if defined(OS_MACOSX)
  maybe_enable_md_.InitWithFeatures(
      {features::kSecondaryUiMd, features::kShowAllDialogsWithViewsToolkit},
      {});
#else
  maybe_enable_md_.InitWithFeatures({features::kSecondaryUiMd}, {});
#endif
}

void TestBrowserDialog::PreShow() {
#if defined(OS_MACOSX)
  // The rest of this method assumes the child dialog is toolkit-views. So, for
  // Mac, it will only work when MD for secondary UI is enabled. Without this, a
  // Cocoa dialog will be created, which TestBrowserDialog doesn't support.
  // Force kSecondaryUiMd on Mac to get coverage on the bots. Leave it optional
  // elsewhere so that the non-MD dialog can be invoked to compare. Note that
  // since SetUp() has already been called, some parts of the toolkit may
  // already be initialized without MD - this is just to ensure Cocoa dialogs
  // are not selected.
  base::test::ScopedFeatureList enable_views_on_mac_always;
  enable_views_on_mac_always.InitWithFeatures(
      {features::kSecondaryUiMd, features::kShowAllDialogsWithViewsToolkit},
      {});
#endif

  GetAllWidgets();
}

bool TestBrowserDialog::VerifyUIPostShow() {
  views::Widget::Widgets widgets_before = widgets_;
  GetAllWidgets();

  auto added = base::STLSetDifference<std::vector<views::Widget*>>(
      widgets_, widgets_before);

  if (added.size() > 1) {
    // Some tests create a standalone window to anchor a dialog. In those cases,
    // ignore added Widgets that are not dialogs.
    base::EraseIf(added, [](views::Widget* widget) {
      return !widget->widget_delegate()->AsDialogDelegate();
    });
  }
  widgets_ = added;

  // This can fail if no dialog was shown, if the dialog shown wasn't a toolkit-
  // views dialog, or if more than one child dialog was shown.
  return added.size() == 1;
}

void TestBrowserDialog::WaitForUserDismissal() {
  ASSERT(!widgets_.empty());
  WidgetObserver observer(widgets_.front());
  observer.WaitForClose();
}

void TestBrowserDialog::DismissUI() {
  ASSERT(!widgets_.empty());
  WidgetCloser closer(widgets_.front(), AlwaysCloseAsynchronously());
  closer.WaitForClose();
}

bool TestBrowserDialog::AlwaysCloseAsynchronously() {
  // TODO(tapted): Iterate over close methods for greater test coverage.
  return false;
}

void TestBrowserDialog::GetAllWidgets() {
  widgets_ = views::test::WidgetTest::GetAllWidgets();
#if defined(OS_CHROMEOS)
  // GetAllWidgets() uses AuraTestHelper to find the aura root window, but
  // that's not used on browser_tests, so ask ash. Under mash the MusClient
  // provides the list of root windows, so this isn't needed.
  if (chromeos::GetAshConfig() != ash::Config::MASH) {
    views::Widget::GetAllChildWidgets(ash::Shell::GetPrimaryRootWindow(),
                                      &widgets_);
  }
#endif  // OS_CHROMEOS
}
