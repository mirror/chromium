// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/system/lock_screen_action/test_lock_screen_action_background_controller.h"

#include "ui/aura/window.h"
#include "ui/views/widget/widget.h"
#include "ui/views/widget/widget_delegate.h"

namespace ash {

namespace {

const char kTestingWidgetName[] = "TestingLockScreenActionBackgroundWidget";

class TestWindowDelegate : public views::WidgetDelegate {
 public:
  explicit TestWindowDelegate(views::Widget* widget) : widget_(widget) {}
  ~TestWindowDelegate() override = default;

  // views::WidgetDelegate:
  void DeleteDelegate() override { delete this; }
  views::Widget* GetWidget() override { return widget_; }
  const views::Widget* GetWidget() const override { return widget_; }
  bool CanActivate() const override { return false; }
  bool CanResize() const override { return true; }
  bool CanMaximize() const override { return true; }
  bool ShouldAdvanceFocusToTopLevelWidget() const override { return true; }

 private:
  views::Widget* widget_ = nullptr;

  DISALLOW_COPY_AND_ASSIGN(TestWindowDelegate);
};

}  // namespace

TestLockScreenActionBackgroundController::
    TestLockScreenActionBackgroundController(bool create_testing_window)
    : create_testing_window_(create_testing_window) {}

TestLockScreenActionBackgroundController::
    ~TestLockScreenActionBackgroundController() = default;

bool TestLockScreenActionBackgroundController::IsBackgroundWindow(
    aura::Window* window) const {
  return window->GetName() == kTestingWidgetName;
}

bool TestLockScreenActionBackgroundController::ShowBackground() {
  if (state() == LockScreenActionBackgroundState::kShowing ||
      state() == LockScreenActionBackgroundState::kShown) {
    return false;
  }

  if (!widget_ && create_testing_window_) {
    widget_ = std::make_unique<views::Widget>();

    views::Widget::InitParams params(views::Widget::InitParams::TYPE_WINDOW);
    params.name = kTestingWidgetName;
    params.parent = parent_window_;
    params.delegate = new TestWindowDelegate(widget_.get());
    params.ownership = views::Widget::InitParams::WIDGET_OWNS_NATIVE_WIDGET;

    widget_->Init(params);
  }

  if (widget_)
    widget_->Show();

  UpdateState(LockScreenActionBackgroundState::kShowing);
  return true;
}

bool TestLockScreenActionBackgroundController::HideBackgroundImmediately() {
  if (state() == LockScreenActionBackgroundState::kHidden)
    return false;

  if (widget_)
    widget_->Hide();
  UpdateState(LockScreenActionBackgroundState::kHidden);
  return true;
}

bool TestLockScreenActionBackgroundController::HideBackground() {
  if (state() == LockScreenActionBackgroundState::kHiding ||
      state() == LockScreenActionBackgroundState::kHidden) {
    return false;
  }

  UpdateState(LockScreenActionBackgroundState::kHiding);
  return true;
}

bool TestLockScreenActionBackgroundController::FinishShow() {
  if (state() != LockScreenActionBackgroundState::kShowing)
    return false;
  UpdateState(LockScreenActionBackgroundState::kShown);
  return true;
}

bool TestLockScreenActionBackgroundController::FinishHide() {
  if (state() != LockScreenActionBackgroundState::kHiding)
    return false;
  if (widget_)
    widget_->Hide();
  UpdateState(LockScreenActionBackgroundState::kHidden);
  return true;
}

const aura::Window* TestLockScreenActionBackgroundController::GetWindow()
    const {
  if (!widget_)
    return nullptr;
  return widget_->GetNativeWindow();
}

}  // namespace ash
