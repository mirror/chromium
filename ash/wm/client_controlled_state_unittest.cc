// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/wm/client_controlled_state.h"

#include "ash/public/cpp/shell_window_ids.h"
#include "ash/shell.h"
#include "ash/test/ash_test_base.h"
#include "ash/wm/window_state.h"
#include "base/macros.h"
#include "ui/views/widget/widget.h"

namespace ash {
namespace wm {
namespace {

constexpr gfx::Rect kInitialBounds(0, 0, 100, 100);

class TestClientControlledStateDelegate
    : public ClientControlledState::Delegate {
 public:
  TestClientControlledStateDelegate() = default;
  ~TestClientControlledStateDelegate() override = default;

  void SendWindowStateRequest(mojom::WindowStateType current_state,
                              mojom::WindowStateType next_state) override {
    old_state_ = current_state;
    new_state_ = next_state;
  }

  void SendBoundsRequest(mojom::WindowStateType current_state,
                         const gfx::Rect& bounds) override {
    requested_bounds_ = bounds;
  }

  mojom::WindowStateType old_state() const { return old_state_; }

  mojom::WindowStateType new_state() const { return new_state_; }

  const gfx::Rect& requested_bounds() const { return requested_bounds_; }

  void reset() {
    old_state_ = mojom::WindowStateType::DEFAULT;
    new_state_ = mojom::WindowStateType::DEFAULT;
    requested_bounds_.SetRect(0, 0, 0, 0);
  }

 private:
  mojom::WindowStateType old_state_ = mojom::WindowStateType::DEFAULT;
  mojom::WindowStateType new_state_ = mojom::WindowStateType::DEFAULT;
  gfx::Rect requested_bounds_;

  DISALLOW_COPY_AND_ASSIGN(TestClientControlledStateDelegate);
};

}  // namespace

class ClientControlledStateTest : public AshTestBase {
 public:
  ClientControlledStateTest() = default;
  ~ClientControlledStateTest() override = default;

  void SetUp() override {
    AshTestBase::SetUp();

    widget_ = std::make_unique<views::Widget>();
    views::Widget::InitParams params;
    params.ownership = views::Widget::InitParams::WIDGET_OWNS_NATIVE_WIDGET;
    params.parent = Shell::GetPrimaryRootWindow()->GetChildById(
        kShellWindowId_DefaultContainer);
    params.bounds = kInitialBounds;
    widget_->Init(params);
    wm::WindowState* window_state = wm::GetWindowState(window());
    auto delegate = std::make_unique<TestClientControlledStateDelegate>();
    delegate_ = delegate.get();
    auto state = std::make_unique<ClientControlledState>(std::move(delegate));
    state_ = state.get();
    window_state->SetStateObject(std::move(state));
    widget_->Show();
  }

  void TearDown() override {
    widget_ = nullptr;
    AshTestBase::TearDown();
  }

 protected:
  aura::Window* window() { return widget_->GetNativeWindow(); }
  WindowState* GetWindowState() { return ::ash::wm::GetWindowState(window()); }
  ClientControlledState* state() { return state_; }
  TestClientControlledStateDelegate* delegate() { return delegate_; }
  views::Widget* widget() { return widget_.get(); }

 private:
  ClientControlledState* state_ = nullptr;
  TestClientControlledStateDelegate* delegate_ = nullptr;
  std::unique_ptr<views::Widget> widget_;

  DISALLOW_COPY_AND_ASSIGN(ClientControlledStateTest);
};

// Make sure that calling Maximize()/Minimize()/Fullscreen() result in
// sending the state change request and won't change the state immediately.
// The state will be updated when ClientControlledState::EnterToNextState
// is called.
TEST_F(ClientControlledStateTest, Maximize) {
  widget()->Maximize();
  // The state shouldn't be updated until EnterToNextState is called.
  EXPECT_FALSE(widget()->IsMaximized());
  EXPECT_EQ(kInitialBounds, widget()->GetWindowBoundsInScreen());
  EXPECT_EQ(mojom::WindowStateType::DEFAULT, delegate()->old_state());
  EXPECT_EQ(mojom::WindowStateType::MAXIMIZED, delegate()->new_state());
  // Now enters the new state.
  state()->EnterToNextState(GetWindowState(), delegate()->new_state());
  EXPECT_TRUE(widget()->IsMaximized());
  // Bounds is controlled by client.
  EXPECT_EQ(kInitialBounds, widget()->GetWindowBoundsInScreen());

  widget()->Restore();
  EXPECT_TRUE(widget()->IsMaximized());
  EXPECT_EQ(kInitialBounds, widget()->GetWindowBoundsInScreen());
  EXPECT_EQ(mojom::WindowStateType::MAXIMIZED, delegate()->old_state());
  EXPECT_EQ(mojom::WindowStateType::NORMAL, delegate()->new_state());
  state()->EnterToNextState(GetWindowState(), delegate()->new_state());
  EXPECT_FALSE(widget()->IsMaximized());
  EXPECT_EQ(kInitialBounds, widget()->GetWindowBoundsInScreen());
}

TEST_F(ClientControlledStateTest, Minimize) {
  widget()->Minimize();
  EXPECT_FALSE(widget()->IsMinimized());
  EXPECT_EQ(kInitialBounds, widget()->GetWindowBoundsInScreen());
  EXPECT_EQ(mojom::WindowStateType::DEFAULT, delegate()->old_state());
  EXPECT_EQ(mojom::WindowStateType::MINIMIZED, delegate()->new_state());
  state()->EnterToNextState(GetWindowState(), delegate()->new_state());
  EXPECT_TRUE(widget()->IsMinimized());
  EXPECT_EQ(kInitialBounds, widget()->GetWindowBoundsInScreen());

  widget()->Restore();
  EXPECT_TRUE(widget()->IsMinimized());
  EXPECT_EQ(kInitialBounds, widget()->GetWindowBoundsInScreen());
  EXPECT_EQ(mojom::WindowStateType::MINIMIZED, delegate()->old_state());
  EXPECT_EQ(mojom::WindowStateType::NORMAL, delegate()->new_state());
  state()->EnterToNextState(GetWindowState(), delegate()->new_state());
  EXPECT_FALSE(widget()->IsMinimized());
  EXPECT_EQ(kInitialBounds, widget()->GetWindowBoundsInScreen());
}

TEST_F(ClientControlledStateTest, Fullscreen) {
  widget()->SetFullscreen(true);
  EXPECT_FALSE(widget()->IsFullscreen());
  EXPECT_EQ(kInitialBounds, widget()->GetWindowBoundsInScreen());
  EXPECT_EQ(mojom::WindowStateType::DEFAULT, delegate()->old_state());
  EXPECT_EQ(mojom::WindowStateType::FULLSCREEN, delegate()->new_state());
  state()->EnterToNextState(GetWindowState(), delegate()->new_state());
  EXPECT_TRUE(widget()->IsFullscreen());
  EXPECT_EQ(kInitialBounds, widget()->GetWindowBoundsInScreen());

  widget()->SetFullscreen(false);
  EXPECT_TRUE(widget()->IsFullscreen());
  EXPECT_EQ(mojom::WindowStateType::FULLSCREEN, delegate()->old_state());
  EXPECT_EQ(mojom::WindowStateType::NORMAL, delegate()->new_state());
  state()->EnterToNextState(GetWindowState(), delegate()->new_state());
  EXPECT_FALSE(widget()->IsFullscreen());
  EXPECT_EQ(kInitialBounds, widget()->GetWindowBoundsInScreen());
}

// Make sure toggle fullscreen from maximized state goes back to
// maximized state.
TEST_F(ClientControlledStateTest, MaximizeToFullscreen) {
  widget()->Maximize();
  EXPECT_FALSE(widget()->IsMaximized());
  EXPECT_EQ(kInitialBounds, widget()->GetWindowBoundsInScreen());
  EXPECT_EQ(mojom::WindowStateType::DEFAULT, delegate()->old_state());
  EXPECT_EQ(mojom::WindowStateType::MAXIMIZED, delegate()->new_state());
  state()->EnterToNextState(GetWindowState(), delegate()->new_state());
  EXPECT_TRUE(widget()->IsMaximized());
  EXPECT_EQ(kInitialBounds, widget()->GetWindowBoundsInScreen());

  widget()->SetFullscreen(true);
  EXPECT_TRUE(widget()->IsMaximized());
  EXPECT_EQ(kInitialBounds, widget()->GetWindowBoundsInScreen());
  EXPECT_EQ(mojom::WindowStateType::MAXIMIZED, delegate()->old_state());
  EXPECT_EQ(mojom::WindowStateType::FULLSCREEN, delegate()->new_state());
  state()->EnterToNextState(GetWindowState(), delegate()->new_state());
  EXPECT_TRUE(widget()->IsFullscreen());
  EXPECT_EQ(kInitialBounds, widget()->GetWindowBoundsInScreen());

  widget()->SetFullscreen(false);
  EXPECT_TRUE(widget()->IsFullscreen());
  EXPECT_EQ(mojom::WindowStateType::FULLSCREEN, delegate()->old_state());
  EXPECT_EQ(mojom::WindowStateType::MAXIMIZED, delegate()->new_state());
  state()->EnterToNextState(GetWindowState(), delegate()->new_state());
  EXPECT_TRUE(widget()->IsMaximized());
  EXPECT_EQ(kInitialBounds, widget()->GetWindowBoundsInScreen());

  widget()->Restore();
  EXPECT_TRUE(widget()->IsMaximized());
  EXPECT_EQ(mojom::WindowStateType::MAXIMIZED, delegate()->old_state());
  EXPECT_EQ(mojom::WindowStateType::NORMAL, delegate()->new_state());
  state()->EnterToNextState(GetWindowState(), delegate()->new_state());
  EXPECT_FALSE(widget()->IsMaximized());
  EXPECT_EQ(kInitialBounds, widget()->GetWindowBoundsInScreen());
}

TEST_F(ClientControlledStateTest, IgnoreWorkspace) {
  widget()->Maximize();
  state()->EnterToNextState(GetWindowState(), delegate()->new_state());
  EXPECT_TRUE(widget()->IsMaximized());
  delegate()->reset();

  UpdateDisplay("1000x800");

  // Client is responsible to handle workspace change, so
  // no action should be taken.
  EXPECT_EQ(kInitialBounds, widget()->GetWindowBoundsInScreen());
  EXPECT_EQ(mojom::WindowStateType::DEFAULT, delegate()->new_state());
  EXPECT_EQ(mojom::WindowStateType::DEFAULT, delegate()->new_state());
  EXPECT_EQ(gfx::Rect(), delegate()->requested_bounds());
}

TEST_F(ClientControlledStateTest, SetBounds) {
  constexpr gfx::Rect new_bounds(100, 100, 100, 100);
  widget()->SetBounds(new_bounds);
  EXPECT_EQ(kInitialBounds, widget()->GetWindowBoundsInScreen());
  EXPECT_EQ(new_bounds, delegate()->requested_bounds());
  state()->set_bounds_locally(true);
  widget()->SetBounds(delegate()->requested_bounds());
  state()->set_bounds_locally(false);
  EXPECT_EQ(new_bounds, widget()->GetWindowBoundsInScreen());
}

}  // namespace wm
}  // namespace ash
