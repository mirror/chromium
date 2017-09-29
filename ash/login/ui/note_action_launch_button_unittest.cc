// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/login/ui/note_action_launch_button.h"

#include <memory>
#include <vector>

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

    data_dispatcher_ = std::make_unique<ash::LoginDataDispatcher>(
        ash::mojom::TrayActionState::kAvailable);
  }

  ash::LoginDataDispatcher* data_dispatcher() { return data_dispatcher_.get(); }

  ash::TestTrayActionClient* tray_action_client() {
    return &tray_action_client_;
  }

  void ShowNoteActionView(ash::NoteActionLaunchButton* note_action) {
    // Wrapp the view inside another view, so it's sized to it's preferred
    // bounds.
    views::View* wrapper = new views::View();
    auto* layout_manager = new views::BoxLayout(views::BoxLayout::kHorizontal);
    layout_manager->set_cross_axis_alignment(
        views::BoxLayout::CROSS_AXIS_ALIGNMENT_START);
    wrapper->SetLayoutManager(layout_manager);

    wrapper->AddChildView(note_action);
    layout_manager->SetFlexForView(note_action, 0);

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
  data_dispatcher()->SetLockScreenNoteState(
      ash::mojom::TrayActionState::kNotAvailable);
  auto note_action_button =
      std::make_unique<ash::NoteActionLaunchButton>(data_dispatcher());
  EXPECT_FALSE(note_action_button->visible());
}

TEST_F(NoteActionLaunchButtonTest, VisibilityActionAvailable) {
  auto note_action_button =
      std::make_unique<ash::NoteActionLaunchButton>(data_dispatcher());
  EXPECT_TRUE(note_action_button->visible());
  EXPECT_TRUE(note_action_button->enabled());

  EXPECT_TRUE(note_action_button->ForegroundViewForTesting()->visible());
  EXPECT_TRUE(note_action_button->ForegroundViewForTesting()->enabled());
  EXPECT_TRUE(note_action_button->BackgroundViewForTesting()->visible());
}

TEST_F(NoteActionLaunchButtonTest, StateChanges) {
  auto scoped_note_action_button =
      std::make_unique<ash::NoteActionLaunchButton>(data_dispatcher());
  auto* note_action_button = scoped_note_action_button.get();
  ShowNoteActionView(scoped_note_action_button.release());

  EXPECT_TRUE(note_action_button->visible());
  EXPECT_TRUE(note_action_button->ForegroundViewForTesting()->visible());
  EXPECT_TRUE(note_action_button->BackgroundViewForTesting()->visible());

  EXPECT_EQ(gfx::Rect(gfx::Point(),
                      gfx::Size(kLargeButtonRadiusDp, kLargeButtonRadiusDp)),
            note_action_button->GetBoundsInScreen());

  data_dispatcher()->SetLockScreenNoteState(
      ash::mojom::TrayActionState::kLaunching);
  EXPECT_FALSE(note_action_button->visible());

  data_dispatcher()->SetLockScreenNoteState(
      ash::mojom::TrayActionState::kActive);
  EXPECT_FALSE(note_action_button->visible());

  data_dispatcher()->SetLockScreenNoteState(
      ash::mojom::TrayActionState::kAvailable);
  EXPECT_TRUE(note_action_button->visible());
  EXPECT_EQ(gfx::Rect(gfx::Point(),
                      gfx::Size(kLargeButtonRadiusDp, kLargeButtonRadiusDp)),
            note_action_button->GetBoundsInScreen());

  data_dispatcher()->SetLockScreenNoteState(
      ash::mojom::TrayActionState::kNotAvailable);
  EXPECT_FALSE(note_action_button->visible());
}

TEST_F(NoteActionLaunchButtonTest, KeyboardTest) {
  auto scoped_note_action_button =
      std::make_unique<ash::NoteActionLaunchButton>(data_dispatcher());
  auto* note_action_button = scoped_note_action_button.get();
  ShowNoteActionView(scoped_note_action_button.release());

  note_action_button->RequestFocus();
  EXPECT_TRUE(note_action_button->ForegroundViewForTesting()->HasFocus());

  ui::test::EventGenerator& generator = GetEventGenerator();
  generator.PressKey(ui::KeyboardCode::VKEY_RETURN, ui::EF_NONE);
  ash::Shell::Get()->tray_action()->FlushMojoForTesting();

  EXPECT_EQ(std::vector<ash::mojom::LockScreenNoteOrigin>(
                {ash::mojom::LockScreenNoteOrigin::kLockScreenButtonKeyboard}),
            tray_action_client()->note_origins());
}

TEST_F(NoteActionLaunchButtonTest, ClickTest) {
  auto scoped_note_action_button =
      std::make_unique<ash::NoteActionLaunchButton>(data_dispatcher());
  auto* note_action_button = scoped_note_action_button.get();
  ShowNoteActionView(scoped_note_action_button.release());

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
