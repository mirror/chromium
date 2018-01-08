// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/views/controls/menu/touchable_menu_root_view.h"

#include <iostream>
#include <vector>

#include "base/strings/utf_string_conversions.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "ui/base/models/simple_menu_model.h"
#include "ui/events/base_event_utils.h"
#include "ui/views/controls/menu/touchable_menu_item_view.h"
#include "ui/views/test/views_test_base.h"
#include "ui/views/vector_icons.h"
#include "ui/views/controls/native/native_view_host.h"
#include "ui/views/view.h"

namespace {

constexpr int kTouchableMenuItemViewWidth = 75;
constexpr int kTouchableMenuItemViewHeight = 10;

}

namespace views {

// A dummy delegate that allows us to ensure the SimpleMenuModel::Delegate
// receives commands.
class TestMenuModelDelegate : public ui::SimpleMenuModel::Delegate {
 public:
  TestMenuModelDelegate() = default;
  ~TestMenuModelDelegate() override = default;

  bool IsCommandIdChecked(int command_id) const override { return false; }

  bool IsCommandIdEnabled(int command_id) const override { return true; }

  void ExecuteCommand(int command_id, int event_flags) override {
    DCHECK(command_id < 10);  // Only support 10 commands for testing.
    executed_commands_[command_id]++;
  }

  int GetCommandIdCount(int command_id) {
    return executed_commands_[command_id];
  }

  int executed_commands_[10];

 private:
  DISALLOW_COPY_AND_ASSIGN(TestMenuModelDelegate);
};

class TouchableMenuRootViewTest : public views::ViewsTestBase,
                                  public testing::WithParamInterface<bool> {
 public:
  TouchableMenuRootViewTest() = default;
  ~TouchableMenuRootViewTest() override = default;

  void SetUp() override {
    ViewsTestBase::SetUp();

    widget_.reset(new Widget());
    Widget::InitParams params;
    params.ownership = Widget::InitParams::WIDGET_OWNS_NATIVE_WIDGET;
    params.context = GetContext();
    widget_->Init(params);
    widget_->Show();

       top_container_ = new View();
          top_container_->SetBounds(0, 0, 1000, 1000);
          top_container_->SetFocusBehavior(View::FocusBehavior::ALWAYS);
          widget_->GetContentsView()->AddChildView(top_container_);

          anchor_view_ = new View();
          anchor_view_->SetBoundsRect(gfx::Rect(0, 0, 300, 300));
          top_container_->AddChildView(anchor_view_);

    if (testing::UnitTest::GetInstance()->current_test_info()->value_param())
      is_touch_ = GetParam();
  }

  void TearDown() override {
    bubble_widget_->Close();
    widget_->Close();
    ViewsTestBase::TearDown();
  }

  void Initialize(int num_items) {
    CreateTestMenuModel(num_items);
    root_view_ =  new TouchableMenuRootView(menu_model_.get(), anchor_view_);
    bubble_widget_ = views::BubbleDialogDelegateView::CreateBubble(root_view_);
    bubble_widget_->Show();
  }

  // TODO(newcomer) press button label here, not the origin.
  void PressButtonLabelAt(int index) {
    TouchableMenuItemView* item_view =
        root_view_->get_touchable_menu_item_view_for_test(index);
    std::unique_ptr<ui::Event> event;

    if (is_touch_) {
      event.reset(new ui::MouseEvent(ui::ET_MOUSE_RELEASED, gfx::Point(),
                                     item_view->GetBoundsInScreen().origin(),
                                     ui::EventTimeForNow(), 0, 0));
    } else {
      event.reset(
          new ui::GestureEvent(0, 0, 0, base::TimeTicks(),
                               ui::GestureEventDetails(ui::ET_GESTURE_TAP)));
    }

    item_view->ButtonPressed(item_view, *event);
  }

  void PressButtonIconAt(int index) {}

  View* contents_view() { return root_view_->child_at(0); }

    TouchableMenuRootView* root_view_;
  std::unique_ptr<TestMenuModelDelegate> test_menu_model_delegate_;
  View* anchor_view_;
  Widget* bubble_widget_;

 private:

  void CreateTestMenuModel(int num_items) {
    test_menu_model_delegate_.reset(new TestMenuModelDelegate());
    menu_model_.reset(new ui::SimpleMenuModel(test_menu_model_delegate_.get()));
    for (int i = 0; i < num_items; i++) {
      menu_model_->AddButton(i, base::ASCIIToUTF16("test"),
                             kCheckboxActiveIcon);
    }
  }

  std::unique_ptr<ui::SimpleMenuModel> menu_model_;
  std::unique_ptr<Widget> widget_; // Owned by the native widget.
  View* top_container_; // Owned by |widget_|'s root-view.
  bool is_touch_ = false;
  DISALLOW_COPY_AND_ASSIGN(TouchableMenuRootViewTest);
};

// Instantiate the Boolean which is used to toggle touch/click in the
// parameterized tests.
INSTANTIATE_TEST_CASE_P(, TouchableMenuRootViewTest, testing::Bool());

  // Tests that the style of the TouchableMenuItemRootView is correct for 1 through 4 children.
TEST_F(TouchableMenuRootViewTest, Basic) {
  LOG(ERROR) << "test";
  EXPECT_EQ(1,1);
  for (int child_count = 1; child_count < 5; child_count++) {
    Initialize(child_count);
    ASSERT_EQ(kTouchableMenuItemViewWidth * child_count,
              contents_view()->bounds().width());
    ASSERT_EQ(kTouchableMenuItemViewHeight, contents_view()->bounds().height());
  }
}

// Tests that providing an empty menu model shows an empty dialog.
TEST_F(TouchableMenuRootViewTest, EmptyMenuModelShowsEmptyDialog) {
  Initialize(0);
  EXPECT_EQ(0, contents_view()->child_count());
  // assert bounds.
}

// Tests that the widget is created properly on show.
TEST_F(TouchableMenuRootViewTest, ShowTouchableMenuRootView) {}

// Tests that clicking or touching the different MenuItemView's activates the
// proper IDs.
TEST_P(TouchableMenuRootViewTest,
       ClickingOrTouchingTouchableMenuItemViewActivatesId) {
  const int num_items = 4;
  Initialize(num_items);
  for (int i = 0; i < num_items; i++) {
    EXPECT_EQ(0, test_menu_model_delegate_->GetCommandIdCount(i));
    LOG(ERROR) << "i: " << i;
    PressButtonLabelAt(i);

    EXPECT_EQ(1, test_menu_model_delegate_->GetCommandIdCount(i));
    PressButtonIconAt(i);
    // Expect...
  }
}

}  // namespace views

/*
 *
 *
 *
 *
 * Fix this:  error: 'NotifyClick' is a protected member of 'views::Button'
 *
 *
 *
 *
 *
 *
 *
 *
 *
 *
 *
 *
 */
