// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/devtools/global_confirm_info_bar.h"

#include <utility>

#include "base/macros.h"
#include "base/memory/ptr_util.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "components/infobars/core/infobar.h"

namespace {

class TestConfirmInfoBarDelegate : public ConfirmInfoBarDelegate {
 public:
  TestConfirmInfoBarDelegate() = default;
  ~TestConfirmInfoBarDelegate() override = default;

  InfoBarIdentifier GetIdentifier() const override { return TEST_INFOBAR; }

  base::string16 GetMessageText() const override {
    return L"GlobalConfirmInfoBar browser tests delegate.";
  }

 private:
  DISALLOW_COPY_AND_ASSIGN(TestConfirmInfoBarDelegate);
};

class GlobalConfirmInfoBarTest : public InProcessBrowserTest {
 public:
  GlobalConfirmInfoBarTest() = default;
  ~GlobalConfirmInfoBarTest() override = default;

 protected:
  InfoBarService* GetInfoBarServiceFromTabIndex(int tab_index) {
    return InfoBarService::FromWebContents(
        browser()->tab_strip_model()->GetWebContentsAt(tab_index));
  }

  // Adds an additional tab.
  void AddTab() {
    AddTabAtIndex(0, GURL(L"chrome://blank/"), ui::PAGE_TRANSITION_LINK);
  }

 private:
  DISALLOW_COPY_AND_ASSIGN(GlobalConfirmInfoBarTest);
};

}  // namespace

// Creates a global confirm info bar on a browser with 4 tabs and closes it.
IN_PROC_BROWSER_TEST_F(GlobalConfirmInfoBarTest, MultipleTabs) {
  TabStripModel* tab_strip_model = browser()->tab_strip_model();

  for (int i = 0; i < 3; i++)
    AddTab();
  ASSERT_EQ(4, browser()->tab_strip_model()->count());

  // Make sure each tabs have no info bars.
  for (int i = 0; i < tab_strip_model->count(); i++) {
    InfoBarService* infobar_service = GetInfoBarServiceFromTabIndex(i);
    EXPECT_EQ(0u, infobar_service->infobar_count());
  }

  auto delegate = base::MakeUnique<TestConfirmInfoBarDelegate>();
  TestConfirmInfoBarDelegate* delegate_ptr = delegate.get();

  base::WeakPtr<GlobalConfirmInfoBar> global_confirm_info_bar =
      GlobalConfirmInfoBar::Show(std::move(delegate));

  // Verify that the info bar is shown on each tab.
  for (int i = 0; i < tab_strip_model->count(); i++) {
    InfoBarService* infobar_service = GetInfoBarServiceFromTabIndex(i);
    ASSERT_EQ(1u, infobar_service->infobar_count());
    EXPECT_TRUE(infobar_service->infobar_at(0u)->delegate()->EqualsDelegate(
        delegate_ptr));
  }

  EXPECT_TRUE(global_confirm_info_bar);
  global_confirm_info_bar->Close();

  EXPECT_FALSE(global_confirm_info_bar);
  for (int i = 0; i < tab_strip_model->count(); i++) {
    EXPECT_EQ(0u, GetInfoBarServiceFromTabIndex(i)->infobar_count());
  }
}

IN_PROC_BROWSER_TEST_F(GlobalConfirmInfoBarTest, UserInteraction) {
  TabStripModel* tab_strip_model = browser()->tab_strip_model();

  for (int i = 0; i < 3; i++)
    AddTab();
  ASSERT_EQ(4, browser()->tab_strip_model()->count());

  // Make sure each tabs have no info bars.
  for (int i = 0; i < tab_strip_model->count(); i++) {
    InfoBarService* infobar_service = GetInfoBarServiceFromTabIndex(i);
    EXPECT_EQ(0u, infobar_service->infobar_count());
  }

  auto delegate = base::MakeUnique<TestConfirmInfoBarDelegate>();
  TestConfirmInfoBarDelegate* delegate_ptr = delegate.get();

  base::WeakPtr<GlobalConfirmInfoBar> global_confirm_info_bar =
      GlobalConfirmInfoBar::Show(std::move(delegate));

  // Verify that the info bar is shown on each tab.
  for (int i = 0; i < tab_strip_model->count(); i++) {
    InfoBarService* infobar_service = GetInfoBarServiceFromTabIndex(i);
    ASSERT_EQ(1u, infobar_service->infobar_count());
    EXPECT_TRUE(infobar_service->infobar_at(0u)->delegate()->EqualsDelegate(
        delegate_ptr));
  }

  // Close the GlobalConfirmInfoBar by interacting with the info bar on one of
  // the tabs. In this case, the first tab is picked.
  EXPECT_TRUE(GetInfoBarServiceFromTabIndex(0)
                  ->infobar_at(0u)
                  ->delegate()
                  ->AsConfirmInfoBarDelegate()
                  ->Accept());

  // The info bar who's button was clicked will be closed by the InfoBarManager,
  // not the GlobalConfirmInfoBar.
  EXPECT_EQ(1u, GetInfoBarServiceFromTabIndex(0)->infobar_count());

  EXPECT_FALSE(global_confirm_info_bar);
  for (int i = 1; i < tab_strip_model->count(); i++) {
    EXPECT_EQ(0u, GetInfoBarServiceFromTabIndex(i)->infobar_count());
  }
}
