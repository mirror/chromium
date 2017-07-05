// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/system/tray/tray_event_filter.h"

#include "ash/system/status_area_widget.h"
#include "ash/system/status_area_widget_delegate.h"
#include "ash/system/tray/system_tray.h"
#include "ash/system/tray/tray_background_view.h"
#include "ash/system/tray/tray_bubble_wrapper.h"
#include "ash/test/ash_test_base.h"
#include "base/macros.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "ui/aura/client/aura_constants.h"
#include "ui/aura/window.h"
#include "ui/views/bubble/tray_bubble_view.h"

namespace ash {
namespace test {

namespace {

class TestTrayBackgroundView : public TrayBackgroundView {
 public:
  TestTrayBackgroundView(Shelf* shelf) : TrayBackgroundView(shelf) {}
  ~TestTrayBackgroundView() override {}

  base::string16 GetAccessibleNameForTray() override {
    return base::string16();
  }
  void HideBubbleWithView(const views::TrayBubbleView* bubble_view) override {}
  void ClickedOutsideBubble() override { click_outside_count_++; }

  int click_outside_count() const { return click_outside_count_; }

 private:
  int click_outside_count_ = 0;

  DISALLOW_COPY_AND_ASSIGN(TestTrayBackgroundView);
};

}  // namespace

class TrayEventFilterTest : public test::AshTestBase {
 public:
  ~TrayEventFilterTest() override {}

  void SetUp() override {
    test::AshTestBase::SetUp();

    keyboard_container_ =
        CreateTestWindow(gfx::Rect(), aura::client::WINDOW_TYPE_NORMAL,
                         kShellWindowId_VirtualKeyboardContainer);
    keyboard_window_ = CreateTestWindow();
    keyboard_window_->set_owned_by_parent(false);
    keyboard_container_->AddChild(keyboard_window_.get());

    menu_container_ =
        CreateTestWindow(gfx::Rect(), aura::client::WINDOW_TYPE_NORMAL,
                         kShellWindowId_MenuContainer);
    menu_window_ = CreateTestWindow();
    menu_window_->set_owned_by_parent(false);
    menu_container_->AddChild(menu_window_.get());

    status_widget_ =
        CreateTestWidget(nullptr, kShellWindowId_StatusContainer, gfx::Rect());
    status_window_ =
        CreateTestWindow(gfx::Rect(), aura::client::WINDOW_TYPE_POPUP);
    status_window_->set_owned_by_parent(false);
    status_widget_->GetNativeView()->AddChild(status_window_.get());
    status_widget_->GetNativeView()->SetProperty(aura::client::kAlwaysOnTopKey,
                                                 true);

    status_area_widget_ =
        new StatusAreaWidget(status_window_.get(), GetPrimaryShelf());
    status_area_widget_->CreateTrayViews();
  }

  void TearDown() override {
    wrapper_.reset();
    bubble_view_.reset();
    tray_view_.reset();
    keyboard_window_.reset();
    keyboard_container_.reset();
    menu_window_.reset();
    menu_container_.reset();
    status_window_.reset();
    status_widget_.reset();
    test::AshTestBase::TearDown();
  }

  TrayBubbleWrapper* wrapper() { return wrapper_.get(); }

  aura::Window* keyboard_window() { return keyboard_window_.get(); }
  aura::Window* menu_window() { return menu_window_.get(); }
  aura::Window* status_window() { return status_window_.get(); }
  StatusAreaWidget* status_area_widget() { return status_area_widget_; }

  gfx::Point outside_point() { return gfx::Point(200, 200); }
  ui::PointerEvent outside_event() {
    const gfx::Point point(200, 200);
    const base::TimeTicks time;
    EXPECT_FALSE(
        status_area_widget()->system_tray()->GetBoundsInScreen().Contains(
            point));
    return ui::PointerEvent(
        ui::MouseEvent(ui::ET_MOUSE_PRESSED, point, point, time, 0, 0));
  }

  gfx::Point inside_point() { return gfx::Point(10, 10); }
  ui::PointerEvent inside_event() {
    const gfx::Point point(10, 10);
    const base::TimeTicks time;
    EXPECT_TRUE(
        status_area_widget()->system_tray()->GetBoundsInScreen().Contains(
            point));
    return ui::PointerEvent(
        ui::MouseEvent(ui::ET_MOUSE_PRESSED, point, point, time, 0, 0));
  }

 private:
  std::unique_ptr<TestTrayBackgroundView> tray_view_;
  std::unique_ptr<views::TrayBubbleView> bubble_view_;
  std::unique_ptr<TrayBubbleWrapper> wrapper_;
  std::unique_ptr<aura::Window> keyboard_container_;
  std::unique_ptr<aura::Window> keyboard_window_;
  std::unique_ptr<aura::Window> menu_container_;
  std::unique_ptr<aura::Window> menu_window_;
  std::unique_ptr<views::Widget> status_widget_;
  std::unique_ptr<aura::Window> status_window_;

  StatusAreaWidget* status_area_widget_;
};

TEST_F(TrayEventFilterTest, ClickingOutsideCloseBubble) {
  StatusAreaWidget* widget = status_area_widget();
  SystemTray* tray = widget->system_tray();
  tray->ShowDefaultView(BUBBLE_CREATE_NEW);
  EXPECT_TRUE(tray->HasSystemBubble());
  EXPECT_TRUE(tray->IsSystemBubbleVisible());

  // Clicking outside should close the bubble.
  TrayEventFilter* filter = tray->tray_event_filter();
  filter->OnPointerEventObserved(outside_event(), outside_point(), nullptr);
  EXPECT_FALSE(tray->IsSystemBubbleVisible());
}

TEST_F(TrayEventFilterTest, ClickingInsideDoesNotCloseBubble) {
  StatusAreaWidget* widget = status_area_widget();
  SystemTray* tray = widget->system_tray();
  tray->ShowDefaultView(BUBBLE_CREATE_NEW);
  EXPECT_TRUE(tray->HasSystemBubble());
  EXPECT_TRUE(tray->IsSystemBubbleVisible());

  // Clicking inside should not close the bubble
  TrayEventFilter* filter = tray->tray_event_filter();
  const gfx::Point point(10, 10);
  const base::TimeTicks time;
  ui::MouseEvent event(ui::ET_MOUSE_PRESSED, point, point, time, 0, 0);
  filter->OnPointerEventObserved(inside_event(), inside_point(), nullptr);
  EXPECT_TRUE(tray->IsSystemBubbleVisible());
}

TEST_F(TrayEventFilterTest, ClickingOnMenuContainerDoesNotCloseBubble) {
  StatusAreaWidget* widget = status_area_widget();
  SystemTray* tray = widget->system_tray();
  tray->ShowDefaultView(BUBBLE_CREATE_NEW);
  EXPECT_TRUE(tray->HasSystemBubble());
  EXPECT_TRUE(tray->IsSystemBubbleVisible());

  // Clicking on MenuContainer should not close the bubble.
  TrayEventFilter* filter = tray->tray_event_filter();
  filter->OnPointerEventObserved(outside_event(), outside_point(),
                                 menu_window());
  EXPECT_TRUE(tray->IsSystemBubbleVisible());
}

TEST_F(TrayEventFilterTest, ClickingOnStatusContainerDoesNotCloseBubble) {
  StatusAreaWidget* widget = status_area_widget();
  SystemTray* tray = widget->system_tray();
  tray->ShowDefaultView(BUBBLE_CREATE_NEW);
  EXPECT_TRUE(tray->HasSystemBubble());
  EXPECT_TRUE(tray->IsSystemBubbleVisible());

  // Clicking on StatusContainer should not close the bubble.
  TrayEventFilter* filter = tray->tray_event_filter();
  filter->OnPointerEventObserved(outside_event(), outside_point(),
                                 status_window());
  EXPECT_TRUE(tray->IsSystemBubbleVisible());
}

TEST_F(TrayEventFilterTest, ClickingOnKeyboardContainerDoesNotCloseBubble) {
  StatusAreaWidget* widget = status_area_widget();
  SystemTray* tray = widget->system_tray();
  tray->ShowDefaultView(BUBBLE_CREATE_NEW);
  EXPECT_TRUE(tray->HasSystemBubble());
  EXPECT_TRUE(tray->IsSystemBubbleVisible());

  // Clicking on KeyboardContainer should not close the bubble.
  TrayEventFilter* filter = tray->tray_event_filter();
  filter->OnPointerEventObserved(outside_event(), outside_point(),
                                 keyboard_window());
  EXPECT_TRUE(tray->IsSystemBubbleVisible());
}

}  // namespace test
}  // namespace ash
