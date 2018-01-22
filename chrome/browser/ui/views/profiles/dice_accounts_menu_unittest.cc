// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/views/profiles/dice_accounts_menu.h"

#include <memory>

#include "chrome/browser/ui/views/harmony/chrome_layout_provider.h"
#include "components/signin/core/browser/account_info.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "ui/views/bubble/bubble_dialog_delegate.h"
#include "ui/views/test/widget_test.h"
#include "ui/views/view.h"
#include "ui/views/widget/widget.h"
#include "ui/views/widget/widget_observer.h"

namespace {

class BubbleView : public views::BubbleDialogDelegateView {
 public:
  BubbleView(views::View* anchor)
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
  }

  void TearDown() override { WidgetTest::TearDown(); }

  void AddWidget(views::Widget* widget) {
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

 private:
  std::set<views::Widget*> widgets_;

  DISALLOW_COPY_AND_ASSIGN(DiceAccountsMenuTest);
};

}  // namespace

// Test to see if |DiceAccountsMenu| closes when it loses focus.
TEST_F(DiceAccountsMenuTest, RequestFocusFromParent) {
  views::Widget* toplevel_widget = CreateTopLevelPlatformWidget();

  views::BubbleDialogDelegateView* parent_bubble =
      new BubbleView(toplevel_widget->GetContentsView());
  parent_bubble->SetFocusBehavior(views::View::FocusBehavior::ALWAYS);
  views::Widget* parent_widget =
      views::BubbleDialogDelegateView::CreateBubble(parent_bubble);
  parent_widget->Show();

  views::Widget* submenu_widget = DiceAccountsMenu::ShowBubble(
      parent_bubble, std::vector<AccountInfo>(), parent_bubble);

  // Add observers to keep track of closed bubbles.
  AddWidget(toplevel_widget);
  AddWidget(parent_widget);
  AddWidget(submenu_widget);

  // Initial checks.
  EXPECT_FALSE(IsWidgetClosed(toplevel_widget));
  EXPECT_FALSE(IsWidgetClosed(parent_widget));
  EXPECT_FALSE(IsWidgetClosed(submenu_widget));

  // Request focus from parent and expect submenu to close.
  parent_bubble->RequestFocus();
  base::RunLoop().RunUntilIdle();

  EXPECT_FALSE(IsWidgetClosed(toplevel_widget));
  EXPECT_FALSE(IsWidgetClosed(parent_widget));
  EXPECT_TRUE(IsWidgetClosed(submenu_widget));

  // Close all openened widgets.
  parent_widget->Close();
  toplevel_widget->Close();
}

// Test to see if |DiceAccountsMenu| closes itself and its parent when another
// window requests the focus, e.g. when the user clicks outside.
TEST_F(DiceAccountsMenuTest, RequestFocusFromTopLevel) {
  views::Widget* toplevel_widget = CreateTopLevelPlatformWidget();
  views::View* toplevel_view = toplevel_widget->GetContentsView();
  toplevel_view->SetFocusBehavior(views::View::FocusBehavior::ALWAYS);

  views::BubbleDialogDelegateView* parent_bubble =
      new BubbleView(toplevel_widget->GetContentsView());
  views::Widget* parent_widget =
      views::BubbleDialogDelegateView::CreateBubble(parent_bubble);
  parent_widget->Show();

  views::Widget* submenu_widget = DiceAccountsMenu::ShowBubble(
      parent_bubble, std::vector<AccountInfo>(), parent_bubble);

  // Add observers to keep track of closed bubbles.
  AddWidget(toplevel_widget);
  AddWidget(parent_widget);
  AddWidget(submenu_widget);

  // Initial checks.
  EXPECT_FALSE(IsWidgetClosed(toplevel_widget));
  EXPECT_FALSE(IsWidgetClosed(parent_widget));
  EXPECT_FALSE(IsWidgetClosed(submenu_widget));

  // Request focus from toplevel window and expect submenu + parent to close.
  toplevel_view->RequestFocus();
  base::RunLoop().RunUntilIdle();

  EXPECT_FALSE(IsWidgetClosed(toplevel_widget));
  EXPECT_TRUE(IsWidgetClosed(parent_widget));
  EXPECT_TRUE(IsWidgetClosed(submenu_widget));

  // Close all openened widgets.
  toplevel_widget->Close();
}
