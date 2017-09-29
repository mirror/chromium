// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/login/ui/note_action_launch_button.h"

#include <memory>
#include <vector>

#include "ash/login/ui/layout_util.h"
#include "ash/login/ui/login_test_base.h"
#include "ash/public/interfaces/tray_action.mojom.h"
#include "ash/shell.h"
#include "ash/tray_action/test_tray_action_client.h"
#include "base/macros.h"
#include "ui/events/test/event_generator.h"
#include "ui/gfx/geometry/point.h"
#include "ui/gfx/geometry/rect.h"
#include "ui/gfx/geometry/vector2d.h"
#include "ui/views/layout/box_layout.h"
#include "ui/views/view.h"

namespace {

// The note action button bubble sizes:
constexpr int kLargeButtonRadiusDp = 56;
constexpr int kSmallButtonRadiusDp = 48;

constexpr float kSqrt2 = 1.4142;

}  // namespace

class NoteActionLaunchButtonTest : public ash::LoginTestBase {
 public:
  NoteActionLaunchButtonTest() = default;
  ~NoteActionLaunchButtonTest() override = default;

  void SetUp() override {
    LoginTestBase::SetUp();

    ash::Shell::Get()->tray_action()->SetClient(
        tray_action_client_.CreateInterfacePtrAndBind(),
        ash::mojom::TrayActionState::kAvailable);
  }

  ash::TestTrayActionClient* tray_action_client() {
    return &tray_action_client_;
  }

  void ShowNoteActionView(ash::NoteActionLaunchButton* note_action) {
    // Wrap the view inside another view, so it's sized to it's preferred
    // bounds.
    views::View* wrapper =
        ash::login_layout_util::WrapViewForPreferredSize(note_action);

    ShowWidgetWithContent(wrapper);
  }

  void PerformClick(const gfx::Point& point) {
    ui::test::EventGenerator& generator = GetEventGenerator();
    generator.MoveMouseTo(point.x(), point.y());
    generator.ClickLeftButton();

    ash::Shell::Get()->tray_action()->FlushMojoForTesting();
  }

 private:
  std::unique_ptr<ash::LoginDataDispatcher> data_dispatcher_;
  ash::TestTrayActionClient tray_action_client_;

  DISALLOW_COPY_AND_ASSIGN(NoteActionLaunchButtonTest);
};

TEST_F(NoteActionLaunchButtonTest, VisibilityActionNotAvailable) {
  auto note_action_button = std::make_unique<ash::NoteActionLaunchButton>(
      ash::mojom::TrayActionState::kNotAvailable, data_dispatcher());
  EXPECT_FALSE(note_action_button->visible());
}

TEST_F(NoteActionLaunchButtonTest, VisibilityActionAvailable) {
  auto note_action_button = std::make_unique<ash::NoteActionLaunchButton>(
      ash::mojom::TrayActionState::kAvailable, data_dispatcher());
  ash::NoteActionLaunchButton::TestApi test_api(note_action_button.get());

  EXPECT_TRUE(note_action_button->visible());
  EXPECT_TRUE(note_action_button->enabled());

  EXPECT_TRUE(test_api.ActionButtonView()->visible());
  EXPECT_TRUE(test_api.ActionButtonView()->enabled());
  EXPECT_TRUE(test_api.BackgroundView()->visible());
}

TEST_F(NoteActionLaunchButtonTest, StateChanges) {
  auto* note_action_button = new ash::NoteActionLaunchButton(
      ash::mojom::TrayActionState::kAvailable, data_dispatcher());
  ShowNoteActionView(note_action_button);
  ash::NoteActionLaunchButton::TestApi test_api(note_action_button);

  // Test goes through different lock screen note state changes and tests that
  // the note action visibility is updated accordingly.

  // In kAvailable state, the action button should be visible.
  EXPECT_TRUE(note_action_button->visible());
  EXPECT_TRUE(test_api.ActionButtonView()->visible());
  EXPECT_TRUE(test_api.BackgroundView()->visible());
  EXPECT_EQ(gfx::Rect(gfx::Point(),
                      gfx::Size(kLargeButtonRadiusDp, kLargeButtonRadiusDp)),
            note_action_button->GetBoundsInScreen());

  // In kLaunching state, the action button should not be visible.
  data_dispatcher()->SetLockScreenNoteState(
      ash::mojom::TrayActionState::kLaunching);
  EXPECT_FALSE(note_action_button->visible());

  // In kActive state, the action button should not be visible.
  data_dispatcher()->SetLockScreenNoteState(
      ash::mojom::TrayActionState::kActive);
  EXPECT_FALSE(note_action_button->visible());

  // When moved back to kAvailable state, the action button should become
  // visible again.
  data_dispatcher()->SetLockScreenNoteState(
      ash::mojom::TrayActionState::kAvailable);
  EXPECT_TRUE(note_action_button->visible());
  EXPECT_EQ(gfx::Rect(gfx::Point(),
                      gfx::Size(kLargeButtonRadiusDp, kLargeButtonRadiusDp)),
            note_action_button->GetBoundsInScreen());

  // In kNotAvailable state, the action button should not be visible.
  data_dispatcher()->SetLockScreenNoteState(
      ash::mojom::TrayActionState::kNotAvailable);
  EXPECT_FALSE(note_action_button->visible());
}

TEST_F(NoteActionLaunchButtonTest, KeyboardTest) {
  auto* note_action_button = new ash::NoteActionLaunchButton(
      ash::mojom::TrayActionState::kAvailable, data_dispatcher());
  ShowNoteActionView(note_action_button);
  ash::NoteActionLaunchButton::TestApi test_api(note_action_button);

  note_action_button->RequestFocus();
  EXPECT_TRUE(test_api.ActionButtonView()->HasFocus());

  ui::test::EventGenerator& generator = GetEventGenerator();
  generator.PressKey(ui::KeyboardCode::VKEY_RETURN, ui::EF_NONE);
  ash::Shell::Get()->tray_action()->FlushMojoForTesting();

  EXPECT_EQ(std::vector<ash::mojom::LockScreenNoteOrigin>(
                {ash::mojom::LockScreenNoteOrigin::kLockScreenButtonKeyboard}),
            tray_action_client()->note_origins());
}

TEST_F(NoteActionLaunchButtonTest, ClickTest) {
  auto* note_action_button = new ash::NoteActionLaunchButton(
      ash::mojom::TrayActionState::kAvailable, data_dispatcher());
  ShowNoteActionView(note_action_button);

  const gfx::Size action_size = note_action_button->GetPreferredSize();
  EXPECT_EQ(gfx::Size(kLargeButtonRadiusDp, kLargeButtonRadiusDp), action_size);
  ASSERT_EQ(gfx::Rect(gfx::Point(), action_size),
            note_action_button->GetBoundsInScreen());

  const gfx::Rect view_bounds = note_action_button->GetBoundsInScreen();
  const std::vector<ash::mojom::LockScreenNoteOrigin> expected_actions = {
      ash::mojom::LockScreenNoteOrigin::kLockScreenButtonTap};

  // The button hit area is expected to be a circle centered in the top right
  // corner of the view with kSmallButtonRadiusDp (and clipped but the view
  // bounds).

  // Point near the center of the view, inside the action button circle:
  PerformClick(view_bounds.top_right() +
               gfx::Vector2d(-kSmallButtonRadiusDp / kSqrt2 + 2,
                             kSmallButtonRadiusDp / kSqrt2 - 2));
  EXPECT_EQ(expected_actions, tray_action_client()->note_origins());
  tray_action_client()->ClearRecordedRequests();

  // Point near the center of the view, outside the action button bubble:
  PerformClick(view_bounds.top_right() +
               gfx::Vector2d(-kSmallButtonRadiusDp / kSqrt2 - 2,
                             kSmallButtonRadiusDp / kSqrt2 + 2));
  EXPECT_TRUE(tray_action_client()->note_origins().empty());
  tray_action_client()->ClearRecordedRequests();

  // Point near top right corner:
  PerformClick(view_bounds.top_right() + gfx::Vector2d(-2, 2));
  EXPECT_EQ(expected_actions, tray_action_client()->note_origins());
  tray_action_client()->ClearRecordedRequests();

  // Point near bottom left corner:
  PerformClick(view_bounds.bottom_left() + gfx::Vector2d(2, -2));
  EXPECT_TRUE(tray_action_client()->note_origins().empty());
  tray_action_client()->ClearRecordedRequests();

  // Point near origin:
  PerformClick(view_bounds.origin() + gfx::Vector2d(2, 2));
  EXPECT_TRUE(tray_action_client()->note_origins().empty());
  tray_action_client()->ClearRecordedRequests();

  // Point near origin of the target area bounding rect (inside the rect):
  PerformClick(view_bounds.top_right() +
               gfx::Vector2d(-kSmallButtonRadiusDp + 2, 2));
  EXPECT_EQ(expected_actions, tray_action_client()->note_origins());
  tray_action_client()->ClearRecordedRequests();

  // Point near origin of the target area bounding rect (outside the rect):
  PerformClick(view_bounds.top_right() +
               gfx::Vector2d(-kSmallButtonRadiusDp - 2, 2));
  EXPECT_TRUE(tray_action_client()->note_origins().empty());
  tray_action_client()->ClearRecordedRequests();

  // Point near bottom right:
  PerformClick(view_bounds.bottom_right() + gfx::Vector2d(0, -2));
  EXPECT_TRUE(tray_action_client()->note_origins().empty());
  tray_action_client()->ClearRecordedRequests();

  // Point near bottom right of the target area bounding rect (inside the rect):
  PerformClick(view_bounds.top_right() +
               gfx::Vector2d(-2, kSmallButtonRadiusDp - 2));
  EXPECT_EQ(expected_actions, tray_action_client()->note_origins());
  tray_action_client()->ClearRecordedRequests();

  // Point near bottom right of the target area bounding rect (outside the
  // rect):
  PerformClick(view_bounds.top_right() +
               gfx::Vector2d(-2, kSmallButtonRadiusDp + 2));
  EXPECT_TRUE(tray_action_client()->note_origins().empty());
  tray_action_client()->ClearRecordedRequests();

  // Point near bottom:
  PerformClick(view_bounds.bottom_left() +
               gfx::Vector2d(kSmallButtonRadiusDp / 2, -1));
  EXPECT_TRUE(tray_action_client()->note_origins().empty());
  tray_action_client()->ClearRecordedRequests();

  // Point near top:
  PerformClick(view_bounds.origin() +
               gfx::Vector2d(kSmallButtonRadiusDp / 2, 1));
  EXPECT_EQ(expected_actions, tray_action_client()->note_origins());
  tray_action_client()->ClearRecordedRequests();

  // Point near left:
  PerformClick(view_bounds.origin() +
               gfx::Vector2d(1, kSmallButtonRadiusDp / 2));
  EXPECT_TRUE(tray_action_client()->note_origins().empty());
  tray_action_client()->ClearRecordedRequests();

  // Point near right:
  PerformClick(view_bounds.top_right() +
               gfx::Vector2d(-1, kSmallButtonRadiusDp / 2));
  EXPECT_EQ(expected_actions, tray_action_client()->note_origins());
  tray_action_client()->ClearRecordedRequests();

  // Point in the center of actionable area:
  PerformClick(
      view_bounds.top_right() +
      gfx::Vector2d(-kSmallButtonRadiusDp / 2, kSmallButtonRadiusDp / 2));
  EXPECT_EQ(expected_actions, tray_action_client()->note_origins());
  tray_action_client()->ClearRecordedRequests();

  // Point outside view bounds:
  PerformClick(view_bounds.top_right() + gfx::Vector2d(2, 2));
  EXPECT_TRUE(tray_action_client()->note_origins().empty());
  tray_action_client()->ClearRecordedRequests();
}
