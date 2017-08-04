// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <utility>

#include "ash/public/cpp/ash_pref_names.h"
#include "ash/public/cpp/shelf_model.h"
#include "ash/public/cpp/shelf_prefs.h"
#include "ash/root_window_controller.h"
#include "ash/session/test_session_controller_client.h"
#include "ash/shelf/shelf.h"
#include "ash/shelf/shelf_button.h"
#include "ash/shelf/shelf_controller.h"
#include "ash/shelf/shelf_view.h"
#include "ash/shelf/shelf_view_test_api.h"
#include "ash/shelf/shelf_widget.h"
#include "ash/shell.h"
#include "ash/test_shell_delegate.h"
#include "ash/test/ash_test_base.h"
#include "ash/test/ash_test_helper.h"
#include "base/stl_util.h"
#include "base/strings/string_number_conversions.h"
#include "components/sync_preferences/testing_pref_service_syncable.h"
#include "mojo/public/cpp/bindings/associated_binding.h"

namespace ash {
namespace {

Shelf* GetShelfForDisplay(int64_t display_id) {
  return Shell::GetRootWindowControllerWithDisplayId(display_id)->shelf();
}

}  // namespace

class ShelfTest : public AshTestBase {
 public:
  ShelfTest() = default;
  ~ShelfTest() override = default;

  // TestingPrefServiceSimple* prefs { return &prefs_; }

  void SetUp() override {
    // Shell::RegisterProfilePrefs(prefs_.registry());
    // ash_test_helper()->set_test_shell_delegate(new TestShellDelegate());
    // ash_test_helper()->test_shell_delegate()->set_active_user_pref_service(&prefs_);

    AshTestBase::SetUp();

    ShelfView* shelf_view = GetPrimaryShelf()->GetShelfViewForTesting();
    shelf_model_ = shelf_view->model();

    test_.reset(new ShelfViewTestAPI(shelf_view));
  }

  ShelfModel* shelf_model() { return shelf_model_; }

  ShelfViewTestAPI* test_api() { return test_.get(); }

 private:
  ShelfModel* shelf_model_ = nullptr;
  std::unique_ptr<ShelfViewTestAPI> test_;
  // TestingPrefServiceSimple prefs_;

  DISALLOW_COPY_AND_ASSIGN(ShelfTest);
};

// Confirms that ShelfItem reflects the appropriated state.
TEST_F(ShelfTest, StatusReflection) {
  // Initially we have the app list.
  int button_count = test_api()->GetButtonCount();

  // Add a running app.
  ShelfItem item;
  item.id = ShelfID("foo");
  item.type = TYPE_APP;
  item.status = STATUS_RUNNING;
  int index = shelf_model()->Add(item);
  ASSERT_EQ(++button_count, test_api()->GetButtonCount());
  ShelfButton* button = test_api()->GetButton(index);
  EXPECT_EQ(ShelfButton::STATE_RUNNING, button->state());

  // Remove it.
  shelf_model()->RemoveItemAt(index);
  ASSERT_EQ(--button_count, test_api()->GetButtonCount());
}

// Confirm that using the menu will clear the hover attribute. To avoid another
// browser test we check this here.
TEST_F(ShelfTest, CheckHoverAfterMenu) {
  // Initially we have the app list.
  int button_count = test_api()->GetButtonCount();

  // Add a running app.
  ShelfItem item;
  item.id = ShelfID("foo");
  item.type = TYPE_APP;
  item.status = STATUS_RUNNING;
  int index = shelf_model()->Add(item);

  ASSERT_EQ(++button_count, test_api()->GetButtonCount());
  ShelfButton* button = test_api()->GetButton(index);
  button->AddState(ShelfButton::STATE_HOVERED);
  button->ShowContextMenu(gfx::Point(), ui::MENU_SOURCE_MOUSE);
  EXPECT_FALSE(button->state() & ShelfButton::STATE_HOVERED);

  // Remove it.
  shelf_model()->RemoveItemAt(index);
}

TEST_F(ShelfTest, ShowOverflowBubble) {
  ShelfWidget* shelf_widget = GetPrimaryShelf()->shelf_widget();

  // Add app buttons until overflow occurs.
  ShelfItem item;
  item.type = TYPE_APP;
  item.status = STATUS_RUNNING;
  while (!test_api()->IsOverflowButtonVisible()) {
    item.id = ShelfID(base::IntToString(shelf_model()->item_count()));
    shelf_model()->Add(item);
    ASSERT_LT(shelf_model()->item_count(), 10000);
  }

  // Shows overflow bubble.
  test_api()->ShowOverflowBubble();
  EXPECT_TRUE(shelf_widget->IsShowingOverflowBubble());

  // Remove one of the first items in the main shelf view.
  ASSERT_GT(shelf_model()->item_count(), 1);
  shelf_model()->RemoveItemAt(1);

  // Waits for all transitions to finish and there should be no crash.
  test_api()->RunMessageLoopUntilAnimationsDone();
  EXPECT_FALSE(shelf_widget->IsShowingOverflowBubble());
}

// Tests that shelf settings respect preference changes.
TEST_F(ShelfTest, ShelfRespectsPrefs) {
  Shelf* shelf = GetPrimaryShelf();
  EXPECT_EQ(SHELF_ALIGNMENT_BOTTOM, shelf->alignment());
  EXPECT_EQ(SHELF_AUTO_HIDE_BEHAVIOR_NEVER, shelf->auto_hide_behavior());

  // TODO(msw|jamescook|xiyuan): Getting a PrefService initialized should be
  // easier; maybe add this to ScopedTestPublicSessionLoginState?
  // TestingPrefServiceSimple pref_service;
  sync_preferences::TestingPrefServiceSyncable pref_service;
  Shell::RegisterProfilePrefs(pref_service.registry());
  ash_test_helper()->test_shell_delegate()->set_active_user_pref_service(&pref_service);
  TestSessionControllerClient* session = GetSessionControllerClient();
  session->AddUserSession("user1@test.com");
  session->SetSessionState(session_manager::SessionState::ACTIVE);
  session->SwitchActiveUser(AccountId::FromUserEmail("user1@test.com"));

  PrefService* prefs = Shell::Get()->GetActiveUserPrefService();
  prefs->SetString(prefs::kShelfAlignmentLocal, "Left");
  EXPECT_EQ("Left", prefs->GetString(prefs::kShelfAlignmentLocal));
  // prefs->SetString(prefs::kShelfAlignmentLocal, "Left");
  // prefs->SetString(prefs::kShelfAlignment, "Left");
  // prefs->SetString(prefs::kShelfAutoHideBehaviorLocal, "Always");
  // prefs->SetString(prefs::kShelfAutoHideBehavior, "Always");
  // base::RunLoop().RunUntilIdle();

  EXPECT_EQ(SHELF_ALIGNMENT_LEFT, shelf->alignment());
  EXPECT_EQ(SHELF_AUTO_HIDE_BEHAVIOR_ALWAYS, shelf->auto_hide_behavior());
}

// Ensure shelf settings are correct after display swap, see crbug.com/748291
TEST_F(ShelfTest, ShelfSettingsValidAfterDisplaySwap) {
  // Simulate adding an external display at the lock screen.
  GetSessionControllerClient()->RequestLockScreen();
  UpdateDisplay("1024x768,800x600");
  base::RunLoop().RunUntilIdle();
  const int64_t internal_display_id = GetPrimaryDisplay().id();
  const int64_t external_display_id = GetSecondaryDisplay().id();

  // The primary shelf should be on the internal display.
  EXPECT_EQ(GetPrimaryShelf(), GetShelfForDisplay(internal_display_id));
  EXPECT_NE(GetPrimaryShelf(), GetShelfForDisplay(external_display_id));

  // Check for the default shelf preferences.
  PrefService* prefs = Shell::Get()->GetActiveUserPrefService();
  EXPECT_EQ(SHELF_AUTO_HIDE_BEHAVIOR_NEVER,
            GetShelfAutoHideBehaviorPref(prefs, internal_display_id));
  EXPECT_EQ(SHELF_AUTO_HIDE_BEHAVIOR_NEVER,
            GetShelfAutoHideBehaviorPref(prefs, external_display_id));
  EXPECT_EQ(SHELF_ALIGNMENT_BOTTOM,
            GetShelfAlignmentPref(prefs, internal_display_id));
  EXPECT_EQ(SHELF_ALIGNMENT_BOTTOM,
            GetShelfAlignmentPref(prefs, external_display_id));

  // Check the current state; shelves have locked alignments in the lock screen.
  EXPECT_EQ(SHELF_AUTO_HIDE_BEHAVIOR_NEVER,
            GetShelfForDisplay(internal_display_id)->auto_hide_behavior());
  EXPECT_EQ(SHELF_AUTO_HIDE_BEHAVIOR_NEVER,
            GetShelfForDisplay(external_display_id)->auto_hide_behavior());
  EXPECT_EQ(SHELF_ALIGNMENT_BOTTOM_LOCKED,
            GetShelfForDisplay(internal_display_id)->alignment());
  EXPECT_EQ(SHELF_ALIGNMENT_BOTTOM_LOCKED,
            GetShelfForDisplay(external_display_id)->alignment());

  // Set some shelf prefs to differentiate the two shelves, check state.
  SetShelfAlignmentPref(prefs, internal_display_id, SHELF_ALIGNMENT_LEFT);
  SetShelfAlignmentPref(prefs, external_display_id, SHELF_ALIGNMENT_RIGHT);
  EXPECT_EQ(SHELF_ALIGNMENT_BOTTOM_LOCKED,
            GetShelfForDisplay(internal_display_id)->alignment());
  EXPECT_EQ(SHELF_ALIGNMENT_BOTTOM_LOCKED,
            GetShelfForDisplay(external_display_id)->alignment());
  SetShelfAutoHideBehaviorPref(prefs, external_display_id,
                               SHELF_AUTO_HIDE_BEHAVIOR_ALWAYS);
  EXPECT_EQ(SHELF_AUTO_HIDE_BEHAVIOR_NEVER,
            GetShelfForDisplay(internal_display_id)->auto_hide_behavior());
  EXPECT_EQ(SHELF_AUTO_HIDE_BEHAVIOR_ALWAYS,
            GetShelfForDisplay(external_display_id)->auto_hide_behavior());

  // Simulate the external display becoming the primary display. The shelves are
  // swapped (each instance now has a different display id), check state.
  SwapPrimaryDisplay();
  base::RunLoop().RunUntilIdle();
  EXPECT_EQ(SHELF_ALIGNMENT_BOTTOM_LOCKED,
            GetShelfForDisplay(internal_display_id)->alignment());
  EXPECT_EQ(SHELF_ALIGNMENT_BOTTOM_LOCKED,
            GetShelfForDisplay(external_display_id)->alignment());
  EXPECT_EQ(SHELF_AUTO_HIDE_BEHAVIOR_ALWAYS,
            GetShelfForDisplay(internal_display_id)->auto_hide_behavior());
  EXPECT_EQ(SHELF_AUTO_HIDE_BEHAVIOR_NEVER,
            GetShelfForDisplay(external_display_id)->auto_hide_behavior());

  // After screen unlock the shelves should have the expected alignment values.
  GetSessionControllerClient()->UnlockScreen();
  base::RunLoop().RunUntilIdle();
  EXPECT_EQ(SHELF_ALIGNMENT_LEFT,
            GetShelfForDisplay(internal_display_id)->alignment());
  EXPECT_EQ(SHELF_ALIGNMENT_RIGHT,
            GetShelfForDisplay(external_display_id)->alignment());
  EXPECT_EQ(SHELF_AUTO_HIDE_BEHAVIOR_NEVER,
            GetShelfForDisplay(internal_display_id)->auto_hide_behavior());
  EXPECT_EQ(SHELF_AUTO_HIDE_BEHAVIOR_ALWAYS,
            GetShelfForDisplay(external_display_id)->auto_hide_behavior());
}

}  // namespace ash
