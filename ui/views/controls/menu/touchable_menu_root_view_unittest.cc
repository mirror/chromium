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

namespace {

constexpr int kTouchableMenuItemViewWidth = 100;
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
    views::ViewsTestBase::SetUp();
    CreateTestWidget();
    if (testing::UnitTest::GetInstance()->current_test_info()->value_param())
      is_touch_ = GetParam();
  }

  void CreateTestWidget() {
    test_widget_.reset(new Widget());
    Widget::InitParams params = CreateParams(Widget::InitParams::TYPE_WINDOW);
    params.ownership = views::Widget::InitParams::WIDGET_OWNS_NATIVE_WIDGET;
    test_widget_->Init(params);
    test_widget_->SetSize(gfx::Size(1000, 1000));
    test_widget_->Show();
  }

  void CreateTestMenuModel(int num_items) {
    test_menu_model_delegate_.reset(new TestMenuModelDelegate());
    menu_model_.reset(new ui::SimpleMenuModel(test_menu_model_delegate_.get()));
    for (int i = 0; i < num_items; i++)
      menu_model_->AddButton(i, base::ASCIIToUTF16("test"),
                             kCheckboxActiveIcon);
  }

  void Initialize(int num_items) {
    CreateTestMenuModel(num_items);
    anchor_view_.reset(new View());
    anchor_view_->SetBoundsRect(gfx::Rect(0, 0, 300, 300));
    root_view_ = new TouchableMenuRootView(
        test_widget_->GetNativeWindow(), menu_model_.get(), anchor_view_.get());
    root_view_->Show();
  }

  void TearDown() override {
    test_widget_->CloseNow();
    views::ViewsTestBase::TearDown();
  }

  // TODO(newcomer) press button label here, not the origin.
  void PressButtonLabelAt(int index) {
    // Maybe make a friend test_api like the shelf, which is able to call
    // NotifyClick on the item.
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
  std::unique_ptr<Widget> test_widget_;
  std::unique_ptr<View> anchor_view_;

 private:
  std::unique_ptr<ui::SimpleMenuModel> menu_model_;
  bool is_touch_ = false;
};

// Instantiate the Boolean which is used to toggle touch/click in the
// parameterized tests.
INSTANTIATE_TEST_CASE_P(, TouchableMenuRootViewTest, testing::Bool());

TEST_F(TouchableMenuRootViewTest, Basic) {
  // Tests that the style of the TouchableMenuItemRootView is correct.
  for (int child_count = 1; child_count < 5; child_count++) {
    Initialize(child_count);
    ASSERT_EQ(kTouchableMenuItemViewWidth * child_count,
              contents_view()->bounds().width());
    ASSERT_EQ(15, contents_view()->bounds().height());
  }
}

// Tests that providing an empty menu model shows an empty dialog.
TEST_F(TouchableMenuRootViewTest, EmptyMenuModelShowsEmptyDialog) {
  Initialize(0);
  ASSERT_EQ(0, contents_view()->child_count());
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
