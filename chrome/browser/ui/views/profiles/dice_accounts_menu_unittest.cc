// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/views/profiles/dice_accounts_menu.h"

#include <memory>

#include "base/strings/utf_string_conversions.h"
#include "chrome/browser/ui/views/hover_button.h"
#include "chrome/browser/ui/views/harmony/chrome_layout_provider.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "ui/accessibility/ax_node_data.h"
#include "ui/gfx/text_utils.h"
#include "ui/views/bubble/bubble_dialog_delegate.h"
#include "ui/views/controls/button/button.h"
#include "ui/views/controls/label.h"
#include "ui/views/focus/focus_manager.h"
#include "ui/views/test/test_views_delegate.h"
#include "ui/views/test/views_test_base.h"
#include "ui/views/test/widget_test.h"
#include "ui/views/view.h"
#include "ui/views/widget/widget.h"

namespace {

class BubbleWithButton : public views::BubbleDialogDelegateView {
 public:
  BubbleWithButton(HoverButton* button, views::View* anchor)
      : BubbleDialogDelegateView(anchor, views::BubbleBorder::NONE) {
    this->AddChildView(button);
  }
};

class DiceAccountsMenuTest : public views::test::WidgetTest {
 public:
  DiceAccountsMenuTest() {}

  void SetUp() override {
    WidgetTest::SetUp();
    desktop_widget_ = CreateTopLevelPlatformWidget();
    test_views_delegate()->set_layout_provider(
        ChromeLayoutProvider::CreateLayoutProvider());
  }

  void TearDown() override {
    desktop_widget_->CloseNow();
    WidgetTest::TearDown();
  }

 private:
  views::Widget* desktop_widget_;

  DISALLOW_COPY_AND_ASSIGN(DiceAccountsMenuTest);
};

}  // namespace

// Still TODO
TEST_F(DiceAccountsMenuTest, TestName) {
  HoverButton* button1 =
      new HoverButton(nullptr, base::ASCIIToUTF16("Button 1"));
  HoverButton* button2 =
      new HoverButton(nullptr, base::ASCIIToUTF16("Button 2"));

  views::Widget* anchor = CreateTopLevelPlatformWidget();
  // Place |button1| and |button2| in different windows.
  views::Widget* bubble1 = BubbleWithButton::CreateBubble(
      new BubbleWithButton(button1, anchor->GetContentsView()));
  //  DiceAccountsMenu::ShowBubble();
  views::Widget* bubble2 = BubbleWithButton::CreateBubble(
      new BubbleWithButton(button2, anchor->GetContentsView()));

  // Show |bubble1| first and then |bubble2|, such that |bubble2| is the active
  // window.
  bubble1->Show();
  bubble2->Show();

  // Check whether the buttons are assigned to the correct widgets.
  EXPECT_EQ(bubble1, button1->GetWidget());
  EXPECT_EQ(bubble2, button2->GetWidget());

  // Since the buttons are in different windows, they should have different
  // focus managers.
  views::FocusManager* manager1 = button1->GetFocusManager();
  views::FocusManager* manager2 = button2->GetFocusManager();
  EXPECT_NE(manager1, manager2);

  // Hover over |button1|. It shouldn't affect the focus.
  button1->SetState(views::Button::ButtonState::STATE_HOVERED);
  EXPECT_NE(manager1->GetFocusedView(), button1);

  // |button2| also shouldn't be focused at this point.
  EXPECT_NE(manager2->GetFocusedView(), button2);

  // Hover over |button2| such that it gets the focus.
  button2->SetState(views::Button::ButtonState::STATE_HOVERED);
  EXPECT_EQ(manager2->GetFocusedView(), button2);

  // Close all showing widgets.
  bubble1->Close();
  bubble2->Close();
  anchor->Close();
}
