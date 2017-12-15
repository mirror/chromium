// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/test/test_browser_ui.h"

#include "base/command_line.h"
#include "base/test/gtest_util.h"
#include "base/test/scoped_feature_list.h"
#include "build/build_config.h"
#include "chrome/common/chrome_features.h"
#include "ui/base/ui_base_features.h"

namespace {

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

TestBrowserUI::TestBrowserUI() = default;
TestBrowserUI::~TestBrowserUI() = default;

void TestBrowserUI::ShowAndVerifyUI() {
  PreShow();
  ShowUI(NameFromTestCase());
  ASSERT_TRUE(VerifyUI());
  if (base::CommandLine::ForCurrentProcess()->HasSwitch(
          internal::kInteractiveSwitch))
    WaitForUserDismissal();
  else
    DismissUI();
}

void TestBrowserUI::UseMdOnly() {
  if (enable_md_)
    return;

  enable_md_ = std::make_unique<base::test::ScopedFeatureList>();
  enable_md_->InitWithFeatures(
#if defined(OS_MACOSX)
      {features::kSecondaryUiMd, features::kShowAllDialogsWithViewsToolkit},
#else
      {features::kSecondaryUiMd},
#endif
      {});
}
