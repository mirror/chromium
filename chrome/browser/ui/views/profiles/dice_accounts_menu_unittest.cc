// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/views/profiles/dice_accounts_menu.h"

#include <memory>

#include "build/build_config.h"
#include "chrome/browser/ui/views/harmony/chrome_layout_provider.h"
#include "components/signin/core/browser/account_info.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "ui/views/bubble/bubble_dialog_delegate.h"
#include "ui/views/test/widget_test.h"
#include "ui/views/view.h"
#include "ui/views/widget/widget.h"
#include "ui/views/widget/widget_observer.h"

class DiceAccountsMenuTestProxy {
 public:
  static views::Widget* GetDiceAccountsMenuWidget() {
    return DiceAccountsMenu::menu_bubble_
               ? DiceAccountsMenu::menu_bubble_->GetWidget()
               : nullptr;
  }
};

namespace {

class BubbleView : public views::BubbleDialogDelegateView {
 public:
  explicit BubbleView(views::View* anchor)
      : BubbleDialogDelegateView(anchor, views::BubbleBorder::NONE) {}
};

class DiceAccountsMenuTest : public views::test::WidgetTest,
                             public views::WidgetObserver {
 public:
  DiceAccountsMenuTest() = default;

  void SetUp() override {
    WidgetTest::SetUp();
    test_views_delegate()->set_layout_provider(
        ChromeLayoutProvider::CreateLayoutProvider());
    toplevel_widget_ = CreateTopLevelPlatformWidget();
  }

  void TearDown() override {
    toplevel_widget_->Close();
    WidgetTest::TearDown();
  }

  void AddObserverTo(views::Widget* widget) {
    widget->AddObserver(this);
    widgets_.insert(widget);
  }

  bool IsWidgetClosed(views::Widget* widget) {
    return widgets_.count(widget) == 0;
  }

  // views::test::WidgetObserver:
  void OnWidgetDestroying(views::Widget* widget) override {
    widgets_.erase(widget);
  }

  views::Widget* toplevel_widget() { return toplevel_widget_; }

 private:
  views::Widget* toplevel_widget_;
  std::set<views::Widget*> widgets_;

  DISALLOW_COPY_AND_ASSIGN(DiceAccountsMenuTest);
};

}  // namespace

#if defined(OS_MACOSX)
// On macOS, the tests do not activate the window on |Show|.
// TODO(http:/crbug.com/805441): Investigate and fix.
#define MAYBE_RequestFocusFromParent DISABLED_RequestFocusFromParent
#define MAYBE_RequestFocusFromTopLevel DISABLED_RequestFocusFromTopLevel
#define MAYBE_ShowBubbleWithDifferentNumOfAccounts \
  DISABLED_ShowBubbleWithDifferentNumOfAccounts
#else
#define MAYBE_RequestFocusFromParent RequestFocusFromParent
#define MAYBE_RequestFocusFromTopLevel RequestFocusFromTopLevel
#define MAYBE_ShowBubbleWithDifferentNumOfAccounts \
  ShowBubbleWithDifferentNumOfAccounts
#endif

// Test to see if |DiceAccountsMenu| closes when it loses focus.
TEST_F(DiceAccountsMenuTest, MAYBE_RequestFocusFromParent) {
  views::BubbleDialogDelegateView* parent_bubble =
      new BubbleView(toplevel_widget()->GetContentsView());
  parent_bubble->SetFocusBehavior(views::View::FocusBehavior::ALWAYS);
  views::Widget* parent_widget =
      views::BubbleDialogDelegateView::CreateBubble(parent_bubble);
  parent_widget->Show();

  DiceAccountsMenu::ShowBubble(parent_bubble, std::vector<AccountInfo>(),
                               parent_bubble);
  views::Widget* submenu_widget =
      DiceAccountsMenuTestProxy::GetDiceAccountsMenuWidget();

  // Add observers to keep track of closed bubbles.
  AddObserverTo(toplevel_widget());
  AddObserverTo(parent_widget);
  AddObserverTo(submenu_widget);

  // Initial checks.
  EXPECT_FALSE(IsWidgetClosed(toplevel_widget()));
  EXPECT_FALSE(IsWidgetClosed(parent_widget));
  EXPECT_FALSE(IsWidgetClosed(submenu_widget));

  // Request focus from parent and expect submenu to close.
  parent_bubble->RequestFocus();
  base::RunLoop().RunUntilIdle();

  EXPECT_FALSE(IsWidgetClosed(toplevel_widget()));
  EXPECT_FALSE(IsWidgetClosed(parent_widget));
  EXPECT_TRUE(IsWidgetClosed(submenu_widget));

  // Close all open widgets.
  parent_widget->Close();
}

// Test to see if |DiceAccountsMenu| closes itself and its parent when another
// window requests the focus, e.g. when the user clicks outside.
TEST_F(DiceAccountsMenuTest, MAYBE_RequestFocusFromTopLevel) {
  views::View* toplevel_view = toplevel_widget()->GetContentsView();
  toplevel_view->SetFocusBehavior(views::View::FocusBehavior::ALWAYS);

  views::BubbleDialogDelegateView* parent_bubble =
      new BubbleView(toplevel_view);
  views::Widget* parent_widget =
      views::BubbleDialogDelegateView::CreateBubble(parent_bubble);
  parent_widget->Show();

  DiceAccountsMenu::ShowBubble(parent_bubble, std::vector<AccountInfo>(),
                               parent_bubble);
  views::Widget* submenu_widget =
      DiceAccountsMenuTestProxy::GetDiceAccountsMenuWidget();

  // Add observers to keep track of closed bubbles.
  AddObserverTo(toplevel_widget());
  AddObserverTo(parent_widget);
  AddObserverTo(submenu_widget);

  // Initial checks.
  EXPECT_FALSE(IsWidgetClosed(toplevel_widget()));
  EXPECT_FALSE(IsWidgetClosed(parent_widget));
  EXPECT_FALSE(IsWidgetClosed(submenu_widget));

  // Request focus from toplevel window and expect submenu + parent to close.
  toplevel_view->RequestFocus();
  base::RunLoop().RunUntilIdle();

  EXPECT_FALSE(IsWidgetClosed(toplevel_widget()));
  EXPECT_TRUE(IsWidgetClosed(parent_widget));
  EXPECT_TRUE(IsWidgetClosed(submenu_widget));
}

// Test to see if |DiceAccountsMenu| can be opened multiple times with different
// number of accounts.
TEST_F(DiceAccountsMenuTest, MAYBE_ShowBubbleWithDifferentNumOfAccounts) {
  views::BubbleDialogDelegateView* parent_bubble =
      new BubbleView(toplevel_widget()->GetContentsView());
  parent_bubble->SetFocusBehavior(views::View::FocusBehavior::ALWAYS);
  views::Widget* parent_widget =
      views::BubbleDialogDelegateView::CreateBubble(parent_bubble);
  parent_widget->Show();

  // Create an accounts list with one account.
  std::vector<AccountInfo> accounts;
  AccountInfo account;
  account.email = "foo1@bar.com";
  accounts.push_back(account);

  // Open the bubble for the first time (with one account).
  DiceAccountsMenu::ShowBubble(parent_bubble, accounts, parent_bubble);

  EXPECT_TRUE(DiceAccountsMenuTestProxy::GetDiceAccountsMenuWidget());

  // Request focus from the parent to close the accounts menu.
  parent_bubble->RequestFocus();
  base::RunLoop().RunUntilIdle();

  EXPECT_FALSE(DiceAccountsMenuTestProxy::GetDiceAccountsMenuWidget());

  // Add a second account to |accounts|.
  account.email = "foo2@bar.com";
  accounts.push_back(account);

  // Open the bubble for the second time (with two accounts).
  DiceAccountsMenu::ShowBubble(parent_bubble, accounts, parent_bubble);

  views::Widget* submenu_widget =
      DiceAccountsMenuTestProxy::GetDiceAccountsMenuWidget();
  EXPECT_TRUE(submenu_widget);

  // Close all open widgets.
  submenu_widget->Close();
  parent_widget->Close();
}
