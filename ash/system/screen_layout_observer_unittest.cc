// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/system/screen_layout_observer.h"

#include "ash/shell.h"
#include "ash/strings/grit/ash_strings.h"
#include "ash/system/tray/system_tray.h"
#include "ash/system/web_notification/web_notification_tray.h"
#include "ash/test/ash_test_base.h"
#include "ash/wm/tablet_mode/tablet_mode_controller.h"
#include "base/command_line.h"
#include "base/strings/string16.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/string_util.h"
#include "base/strings/utf_string_conversions.h"
#include "ui/accessibility/ax_node_data.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/chromeos/devicetype_utils.h"
#include "ui/display/display.h"
#include "ui/display/display_layout_builder.h"
#include "ui/display/display_switches.h"
#include "ui/display/manager/display_manager.h"
#include "ui/display/manager/display_manager_utilities.h"
#include "ui/display/test/display_manager_test_api.h"
#include "ui/message_center/message_center.h"
#include "ui/message_center/notification_list.h"
#include "ui/message_center/public/cpp/notification.h"
#include "ui/views/controls/label.h"

namespace ash {

class ScreenLayoutObserverTest : public AshTestBase {
 public:
  ScreenLayoutObserverTest();
  ~ScreenLayoutObserverTest() override;

 protected:
  void SetUp() override {
    AshTestBase::SetUp();
    WebNotificationTray::DisableAnimationsForTest(true);
  }

  void TearDown() override {
    WebNotificationTray::DisableAnimationsForTest(false);
    AshTestBase::TearDown();
  }

  ScreenLayoutObserver* GetScreenLayoutObserver();
  void CheckUpdate();

  void CloseNotification();
  base::string16 GetDisplayNotificationText() const;
  base::string16 GetDisplayNotificationAdditionalText() const;

  base::string16 GetFirstDisplayName();

  base::string16 GetSecondDisplayName();

  base::string16 GetMirroringDisplayNames();

  base::string16 GetUnifiedDisplayName();

 private:
  const message_center::Notification* GetDisplayNotification() const;

  DISALLOW_COPY_AND_ASSIGN(ScreenLayoutObserverTest);
};

ScreenLayoutObserverTest::ScreenLayoutObserverTest() = default;

ScreenLayoutObserverTest::~ScreenLayoutObserverTest() = default;

ScreenLayoutObserver* ScreenLayoutObserverTest::GetScreenLayoutObserver() {
  return Shell::Get()->screen_layout_observer();
}

void ScreenLayoutObserverTest::CloseNotification() {
  message_center::MessageCenter::Get()->RemoveNotification(
      ScreenLayoutObserver::kNotificationId, false);
  RunAllPendingInMessageLoop();
}

base::string16 ScreenLayoutObserverTest::GetDisplayNotificationText() const {
  const message_center::Notification* notification = GetDisplayNotification();
  return notification ? notification->title() : base::string16();
}

base::string16 ScreenLayoutObserverTest::GetDisplayNotificationAdditionalText()
    const {
  const message_center::Notification* notification = GetDisplayNotification();
  return notification ? notification->message() : base::string16();
}

base::string16 ScreenLayoutObserverTest::GetFirstDisplayName() {
  return base::UTF8ToUTF16(display_manager()->GetDisplayNameForId(
      display_manager()->first_display_id()));
}

base::string16 ScreenLayoutObserverTest::GetSecondDisplayName() {
  return base::UTF8ToUTF16(display_manager()->GetDisplayNameForId(
      display_manager()->GetSecondaryDisplay().id()));
}

base::string16 ScreenLayoutObserverTest::GetMirroringDisplayNames() {
  DCHECK(display_manager()->IsInMirrorMode());
  base::string16 display_names;
  for (auto& id : display_manager()->GetMirroringDestinationDisplayIdList()) {
    if (!display_names.empty())
      display_names.append(base::UTF8ToUTF16(","));
    display_names.append(
        base::UTF8ToUTF16(display_manager()->GetDisplayNameForId(id)));
  }
  return display_names;
}

base::string16 ScreenLayoutObserverTest::GetUnifiedDisplayName() {
  return base::UTF8ToUTF16(
      display_manager()->GetDisplayNameForId(display::kUnifiedDisplayId));
}

const message_center::Notification*
ScreenLayoutObserverTest::GetDisplayNotification() const {
  const message_center::NotificationList::Notifications notifications =
      message_center::MessageCenter::Get()->GetVisibleNotifications();
  for (const auto* notification : notifications) {
    if (notification->id() == ScreenLayoutObserver::kNotificationId)
      return notification;
  }

  return nullptr;
}

class ScreenLayoutObserverTestMultiMirroring
    : public ScreenLayoutObserverTest,
      public testing::WithParamInterface<bool> {
 public:
  ScreenLayoutObserverTestMultiMirroring() = default;
  ~ScreenLayoutObserverTestMultiMirroring() override = default;

 protected:
  void SetUp() override {
    bool should_disable_multi_mirroring = GetParam();
    if (should_disable_multi_mirroring) {
      base::CommandLine::ForCurrentProcess()->AppendSwitch(
          ::switches::kDisableMultiMirroring);
    }
    ScreenLayoutObserverTest::SetUp();
  }

 private:
  DISALLOW_COPY_AND_ASSIGN(ScreenLayoutObserverTestMultiMirroring);
};

// Instantiate the boolean which is used to enable/disable multi-mirroring in
// the parameterized tests.
INSTANTIATE_TEST_CASE_P(,
                        ScreenLayoutObserverTestMultiMirroring,
                        testing::Bool());

TEST_P(ScreenLayoutObserverTestMultiMirroring, DisplayNotifications) {
  Shell::Get()->screen_layout_observer()->set_show_notifications_for_testing(
      true);

  UpdateDisplay("400x400");
  display::Display::SetInternalDisplayId(display_manager()->first_display_id());
  EXPECT_TRUE(GetDisplayNotificationText().empty());

  // rotation.
  UpdateDisplay("400x400/r");
  EXPECT_EQ(l10n_util::GetStringFUTF16(
                IDS_ASH_STATUS_TRAY_DISPLAY_ROTATED, GetFirstDisplayName(),
                l10n_util::GetStringUTF16(
                    IDS_ASH_STATUS_TRAY_DISPLAY_ORIENTATION_90)),
            GetDisplayNotificationAdditionalText());
  EXPECT_TRUE(GetDisplayNotificationText().empty());

  CloseNotification();
  UpdateDisplay("400x400");
  EXPECT_EQ(l10n_util::GetStringFUTF16(
                IDS_ASH_STATUS_TRAY_DISPLAY_ROTATED, GetFirstDisplayName(),
                l10n_util::GetStringUTF16(
                    IDS_ASH_STATUS_TRAY_DISPLAY_STANDARD_ORIENTATION)),
            GetDisplayNotificationAdditionalText());
  EXPECT_TRUE(GetDisplayNotificationText().empty());

  // UI-scale
  CloseNotification();
  UpdateDisplay("400x400@1.5");
  EXPECT_EQ(l10n_util::GetStringFUTF16(
                IDS_ASH_STATUS_TRAY_DISPLAY_RESOLUTION_CHANGED,
                GetFirstDisplayName(), base::UTF8ToUTF16("600x600")),
            GetDisplayNotificationAdditionalText());
  EXPECT_EQ(l10n_util::GetStringUTF16(
                IDS_ASH_STATUS_TRAY_DISPLAY_RESOLUTION_CHANGED_TITLE),
            GetDisplayNotificationText());

  // UI-scale to 1.0
  CloseNotification();
  UpdateDisplay("400x400");
  EXPECT_EQ(l10n_util::GetStringFUTF16(
                IDS_ASH_STATUS_TRAY_DISPLAY_RESOLUTION_CHANGED,
                GetFirstDisplayName(), base::UTF8ToUTF16("400x400")),
            GetDisplayNotificationAdditionalText());
  EXPECT_EQ(l10n_util::GetStringUTF16(
                IDS_ASH_STATUS_TRAY_DISPLAY_RESOLUTION_CHANGED_TITLE),
            GetDisplayNotificationText());

  // No-update
  CloseNotification();
  UpdateDisplay("400x400");
  EXPECT_TRUE(GetDisplayNotificationText().empty());
  EXPECT_TRUE(GetDisplayNotificationAdditionalText().empty());

  // Extended.
  CloseNotification();
  UpdateDisplay("400x400,200x200");
  EXPECT_EQ(l10n_util::GetStringFUTF16(IDS_ASH_STATUS_TRAY_DISPLAY_EXTENDED,
                                       GetSecondDisplayName()),
            GetDisplayNotificationText());
  EXPECT_TRUE(GetDisplayNotificationAdditionalText().empty());

  const int64_t first_display_id =
      display::Screen::GetScreen()->GetPrimaryDisplay().id();
  const int64_t second_display_id = first_display_id + 1;
  display::ManagedDisplayInfo first_display_info =
      display::CreateDisplayInfo(first_display_id, gfx::Rect(1, 1, 500, 500));
  display::ManagedDisplayInfo second_display_info =
      display::CreateDisplayInfo(second_display_id, gfx::Rect(2, 2, 500, 500));
  std::vector<display::ManagedDisplayInfo> display_info_list;
  display_info_list.emplace_back(first_display_info);
  display_info_list.emplace_back(second_display_info);
  display_manager()->OnNativeDisplaysChanged(display_info_list);

  // Turn on mirror mode.
  CloseNotification();
  display_manager()->SetMirrorMode(display::MirrorMode::kNormal, base::nullopt);
  EXPECT_EQ(l10n_util::GetStringFUTF16(IDS_ASH_STATUS_TRAY_DISPLAY_MIRRORING,
                                       GetMirroringDisplayNames()),
            GetDisplayNotificationText());
  EXPECT_TRUE(GetDisplayNotificationAdditionalText().empty());

  // Disconnect a display to end mirror mode.
  CloseNotification();
  display_info_list.erase(display_info_list.end() - 1);
  display_manager()->OnNativeDisplaysChanged(display_info_list);
  EXPECT_EQ(l10n_util::GetStringUTF16(IDS_ASH_STATUS_TRAY_DISPLAY_MIRROR_EXIT),
            GetDisplayNotificationText());
  EXPECT_TRUE(GetDisplayNotificationAdditionalText().empty());

  // Restore mirror mode.
  CloseNotification();
  display_info_list.emplace_back(second_display_info);
  display_manager()->OnNativeDisplaysChanged(display_info_list);
  EXPECT_EQ(l10n_util::GetStringFUTF16(IDS_ASH_STATUS_TRAY_DISPLAY_MIRRORING,
                                       GetMirroringDisplayNames()),
            GetDisplayNotificationText());
  EXPECT_TRUE(GetDisplayNotificationAdditionalText().empty());

  // Turn off mirror mode.
  CloseNotification();
  display_manager()->SetMirrorMode(display::MirrorMode::kOff, base::nullopt);
  if (display_manager()->is_multi_mirroring_enabled()) {
    EXPECT_EQ(
        l10n_util::GetStringUTF16(IDS_ASH_STATUS_TRAY_DISPLAY_MIRROR_EXIT),
        GetDisplayNotificationText());
  } else {
    EXPECT_EQ(l10n_util::GetStringFUTF16(IDS_ASH_STATUS_TRAY_DISPLAY_EXTENDED,
                                         GetSecondDisplayName()),
              GetDisplayNotificationText());
  }
  EXPECT_TRUE(GetDisplayNotificationAdditionalText().empty());

  // Resize the first display.
  UpdateDisplay("400x400@1.5,200x200");
  EXPECT_EQ(l10n_util::GetStringFUTF16(
                IDS_ASH_STATUS_TRAY_DISPLAY_RESOLUTION_CHANGED,
                GetFirstDisplayName(), base::UTF8ToUTF16("600x600")),
            GetDisplayNotificationAdditionalText());
  EXPECT_EQ(l10n_util::GetStringUTF16(
                IDS_ASH_STATUS_TRAY_DISPLAY_RESOLUTION_CHANGED_TITLE),
            GetDisplayNotificationText());

  // Rotate the second.
  UpdateDisplay("400x400@1.5,200x200/r");
  EXPECT_EQ(l10n_util::GetStringFUTF16(
                IDS_ASH_STATUS_TRAY_DISPLAY_ROTATED, GetSecondDisplayName(),
                l10n_util::GetStringUTF16(
                    IDS_ASH_STATUS_TRAY_DISPLAY_ORIENTATION_90)),
            GetDisplayNotificationAdditionalText());
  EXPECT_TRUE(GetDisplayNotificationText().empty());

  // Enters closed lid mode.
  UpdateDisplay("400x400@1.5,200x200");
  display::Display::SetInternalDisplayId(
      display_manager()->GetSecondaryDisplay().id());
  UpdateDisplay("400x400@1.5");
  EXPECT_TRUE(GetDisplayNotificationText().empty());
}

// Zooming in Unified Mode results in display size changes rather than changes
// in the UI scales, in which case, we still want to show a notification when
// the source of change is not the settings ui.
TEST_F(ScreenLayoutObserverTest, ZoomingInUnifiedModeNotification) {
  Shell::Get()->screen_layout_observer()->set_show_notifications_for_testing(
      true);
  UpdateDisplay("400x400,200x200");

  // Enter unified mode.
  display_manager()->SetUnifiedDesktopEnabled(true);

  // Using keyboard shortcuts to change the zoom should result in a
  // notification.
  CloseNotification();
  EXPECT_TRUE(display_manager()->ZoomInternalDisplay(false /* up */));
  EXPECT_EQ(l10n_util::GetStringFUTF16(
                IDS_ASH_STATUS_TRAY_DISPLAY_RESOLUTION_CHANGED,
                GetUnifiedDisplayName(), base::UTF8ToUTF16("400x200")),
            GetDisplayNotificationAdditionalText());
  EXPECT_EQ(l10n_util::GetStringUTF16(
                IDS_ASH_STATUS_TRAY_DISPLAY_RESOLUTION_CHANGED_TITLE),
            GetDisplayNotificationText());

  CloseNotification();
  EXPECT_TRUE(display_manager()->ZoomInternalDisplay(true /* up */));
  EXPECT_EQ(l10n_util::GetStringFUTF16(
                IDS_ASH_STATUS_TRAY_DISPLAY_RESOLUTION_CHANGED,
                GetUnifiedDisplayName(), base::UTF8ToUTF16("800x400")),
            GetDisplayNotificationAdditionalText());
  EXPECT_EQ(l10n_util::GetStringUTF16(
                IDS_ASH_STATUS_TRAY_DISPLAY_RESOLUTION_CHANGED_TITLE),
            GetDisplayNotificationText());

  // However, when the source is the settings UI, the ScreenLayoutObserver does
  // not produce a notification for resolution changes in Unified Mode. These
  // are handled by the ResolutionNotificationController instead.
  CloseNotification();
  Shell::Get()->screen_layout_observer()->SetDisplayChangedFromSettingsUI(
      display::kUnifiedDisplayId);
  EXPECT_TRUE(display_manager()->ZoomInternalDisplay(false /* up */));
  EXPECT_TRUE(GetDisplayNotificationAdditionalText().empty());
  EXPECT_TRUE(GetDisplayNotificationText().empty());
}

// Verify that notification shows up when display is switched from dock mode to
// extend mode.
TEST_F(ScreenLayoutObserverTest, DisplayConfigurationChangedTwice) {
  Shell::Get()->screen_layout_observer()->set_show_notifications_for_testing(
      true);
  UpdateDisplay("400x400,200x200");
  EXPECT_EQ(l10n_util::GetStringUTF16(
                IDS_ASH_STATUS_TRAY_DISPLAY_EXTENDED_NO_INTERNAL),
            GetDisplayNotificationText());

  // OnDisplayConfigurationChanged() may be called more than once for a single
  // update display in case of primary is swapped or recovered from dock mode.
  // Should not remove the notification in such case.
  GetScreenLayoutObserver()->OnDisplayConfigurationChanged();
  EXPECT_EQ(l10n_util::GetStringUTF16(
                IDS_ASH_STATUS_TRAY_DISPLAY_EXTENDED_NO_INTERNAL),
            GetDisplayNotificationText());

  // Back to the single display. It should show that a display was removed.
  UpdateDisplay("400x400");
  EXPECT_TRUE(base::StartsWith(
      GetDisplayNotificationText(),
      l10n_util::GetStringFUTF16(IDS_ASH_STATUS_TRAY_DISPLAY_REMOVED,
                                 base::UTF8ToUTF16("")),
      base::CompareCase::SENSITIVE));
}

// Verify the notification message content when one of the 2 displays that
// connected to the device is rotated.
TEST_F(ScreenLayoutObserverTest, UpdateAfterSuppressDisplayNotification) {
  UpdateDisplay("400x400,200x200");

  Shell::Get()->screen_layout_observer()->set_show_notifications_for_testing(
      true);

  // Rotate the second.
  UpdateDisplay("400x400,200x200/r");
  EXPECT_EQ(l10n_util::GetStringFUTF16(
                IDS_ASH_STATUS_TRAY_DISPLAY_ROTATED, GetSecondDisplayName(),
                l10n_util::GetStringUTF16(
                    IDS_ASH_STATUS_TRAY_DISPLAY_ORIENTATION_90)),
            GetDisplayNotificationAdditionalText());
}

// Verify that no notification is shown when overscan of a screen is changed.
TEST_F(ScreenLayoutObserverTest, OverscanDisplay) {
  UpdateDisplay("400x400, 300x300");
  Shell::Get()->screen_layout_observer()->set_show_notifications_for_testing(
      true);
  display::Display::SetInternalDisplayId(display_manager()->first_display_id());

  // /o creates the default overscan.
  UpdateDisplay("400x400, 300x300/o");
  EXPECT_TRUE(GetDisplayNotificationText().empty());
  EXPECT_TRUE(GetDisplayNotificationAdditionalText().empty());

  // Reset the overscan.
  Shell::Get()->display_manager()->SetOverscanInsets(
      display_manager()->GetSecondaryDisplay().id(), gfx::Insets());
  EXPECT_TRUE(GetDisplayNotificationText().empty());
  EXPECT_TRUE(GetDisplayNotificationAdditionalText().empty());
}

// Tests that exiting mirror mode by closing the lid shows the correct "exiting
// mirror mode" message.
TEST_F(ScreenLayoutObserverTest, ExitMirrorModeBecauseOfDockedModeMessage) {
  Shell::Get()->screen_layout_observer()->set_show_notifications_for_testing(
      true);
  UpdateDisplay("400x400,200x200");
  display::Display::SetInternalDisplayId(
      display_manager()->GetSecondaryDisplay().id());

  // Mirroring.
  UpdateDisplay("400x400,200x200");
  display_manager()->SetMirrorMode(display::MirrorMode::kNormal, base::nullopt);
  EXPECT_EQ(l10n_util::GetStringFUTF16(IDS_ASH_STATUS_TRAY_DISPLAY_MIRRORING,
                                       GetMirroringDisplayNames()),
            GetDisplayNotificationText());

  // Docked.
  CloseNotification();
  display::Display::SetInternalDisplayId(display_manager()->first_display_id());
  UpdateDisplay("200x200");
  EXPECT_EQ(l10n_util::GetStringUTF16(IDS_ASH_STATUS_TRAY_DISPLAY_MIRROR_EXIT),
            GetDisplayNotificationText());
}

// TODO(crbug.com/774795) Remove this test when multi mirroring is enabled by
// default.
// Tests that exiting mirror mode because of adding a third display shows the
// correct "3+ displays mirror mode is not supported" message.
TEST_P(ScreenLayoutObserverTestMultiMirroring,
       ExitMirrorModeBecauseOfThirdDisplayMessage) {
  if (display_manager()->is_multi_mirroring_enabled()) {
    // This test is not neccessary when mirroring across 3+ displays is
    // supported.
    return;
  }
  Shell::Get()->screen_layout_observer()->set_show_notifications_for_testing(
      true);
  UpdateDisplay("400x400,200x200");
  display::Display::SetInternalDisplayId(
      display_manager()->GetSecondaryDisplay().id());

  // Mirroring.
  UpdateDisplay("400x400,200x200");
  display_manager()->SetMirrorMode(display::MirrorMode::kNormal, base::nullopt);
  EXPECT_EQ(l10n_util::GetStringFUTF16(IDS_ASH_STATUS_TRAY_DISPLAY_MIRRORING,
                                       GetMirroringDisplayNames()),
            GetDisplayNotificationText());

  // Adding a third display. Mirror mode for 3+ displays is not supported.
  CloseNotification();
  UpdateDisplay("400x400,200x200,100x100");
  EXPECT_EQ(l10n_util::GetStringUTF16(IDS_ASH_DISPLAY_MIRRORING_NOT_SUPPORTED),
            GetDisplayNotificationText());
}

// Special case: tests that exiting mirror mode by removing a display shows the
// correct message.
TEST_F(ScreenLayoutObserverTest,
       ExitMirrorModeNoInternalDisplayBecauseOfDisplayRemovedMessage) {
  Shell::Get()->screen_layout_observer()->set_show_notifications_for_testing(
      true);
  UpdateDisplay("400x400,200x200");
  display::Display::SetInternalDisplayId(
      display_manager()->GetSecondaryDisplay().id());

  // Mirroring.
  UpdateDisplay("400x400,200x200");
  display_manager()->SetMirrorMode(display::MirrorMode::kNormal, base::nullopt);
  EXPECT_EQ(l10n_util::GetStringFUTF16(IDS_ASH_STATUS_TRAY_DISPLAY_MIRRORING,
                                       GetMirroringDisplayNames()),
            GetDisplayNotificationText());

  // Removing one of the displays. We show that we exited mirror mode.
  CloseNotification();
  UpdateDisplay("400x400");
  EXPECT_EQ(l10n_util::GetStringUTF16(IDS_ASH_STATUS_TRAY_DISPLAY_MIRROR_EXIT),
            GetDisplayNotificationText());
}

// Tests notification messages shown when adding and removing displays in
// extended mode.
TEST_F(ScreenLayoutObserverTest, AddingRemovingDisplayExtendedModeMessage) {
  Shell::Get()->screen_layout_observer()->set_show_notifications_for_testing(
      true);
  UpdateDisplay("400x400");
  EXPECT_TRUE(GetDisplayNotificationText().empty());

  // Adding a display in extended mode.
  UpdateDisplay("400x400,200x200");
  EXPECT_EQ(l10n_util::GetStringUTF16(
                IDS_ASH_STATUS_TRAY_DISPLAY_EXTENDED_NO_INTERNAL),
            GetDisplayNotificationText());

  // Removing a display.
  CloseNotification();
  UpdateDisplay("400x400");
  EXPECT_TRUE(base::StartsWith(
      GetDisplayNotificationText(),
      l10n_util::GetStringFUTF16(IDS_ASH_STATUS_TRAY_DISPLAY_REMOVED,
                                 base::UTF8ToUTF16("")),
      base::CompareCase::SENSITIVE));
}

// Tests notification messages shown when entering and exiting unified desktop
// mode.
TEST_F(ScreenLayoutObserverTest, EnteringExitingUnifiedModeMessage) {
  Shell::Get()->screen_layout_observer()->set_show_notifications_for_testing(
      true);
  UpdateDisplay("400x400");
  EXPECT_TRUE(GetDisplayNotificationText().empty());

  // Adding a display in extended mode.
  UpdateDisplay("400x400,200x200");
  EXPECT_EQ(l10n_util::GetStringUTF16(
                IDS_ASH_STATUS_TRAY_DISPLAY_EXTENDED_NO_INTERNAL),
            GetDisplayNotificationText());

  // Enter unified mode.
  display_manager()->SetUnifiedDesktopEnabled(true);
  EXPECT_EQ(l10n_util::GetStringUTF16(IDS_ASH_STATUS_TRAY_DISPLAY_UNIFIED),
            GetDisplayNotificationText());

  // Exit unified mode.
  display_manager()->SetUnifiedDesktopEnabled(false);
  EXPECT_EQ(
      l10n_util::GetStringUTF16(IDS_ASH_STATUS_TRAY_DISPLAY_UNIFIED_EXITING),
      GetDisplayNotificationText());

  // Enter unified mode again and exit via closing the lid. The message "Exiting
  // unified mode" should be shown.
  display_manager()->SetUnifiedDesktopEnabled(true);
  EXPECT_EQ(l10n_util::GetStringUTF16(IDS_ASH_STATUS_TRAY_DISPLAY_UNIFIED),
            GetDisplayNotificationText());

  // Close the lid.
  display::Display::SetInternalDisplayId(display_manager()->first_display_id());
  UpdateDisplay("200x200");
  display_manager()->SetUnifiedDesktopEnabled(false);
  EXPECT_EQ(
      l10n_util::GetStringUTF16(IDS_ASH_STATUS_TRAY_DISPLAY_UNIFIED_EXITING),
      GetDisplayNotificationText());
}

// Special case: Tests notification messages shown when entering docked mode
// by closing the lid and the internal display is the secondary display.
TEST_F(ScreenLayoutObserverTest, DockedModeWithExternalPrimaryDisplayMessage) {
  Shell::Get()->screen_layout_observer()->set_show_notifications_for_testing(
      true);
  UpdateDisplay("400x400,200x200");
  EXPECT_EQ(l10n_util::GetStringUTF16(
                IDS_ASH_STATUS_TRAY_DISPLAY_EXTENDED_NO_INTERNAL),
            GetDisplayNotificationText());
  CloseNotification();

  const int64_t primary_id = display_manager()->GetDisplayAt(0).id();
  const int64_t internal_secondary_id = display_manager()->GetDisplayAt(1).id();
  display::Display::SetInternalDisplayId(internal_secondary_id);
  display::DisplayLayoutBuilder builder(primary_id);
  builder.AddDisplayPlacement(internal_secondary_id, primary_id,
                              display::DisplayPlacement::LEFT, 0);
  display_manager()->SetLayoutForCurrentDisplays(builder.Build());
  EXPECT_TRUE(GetDisplayNotificationText().empty());

  // Close the lid. We go to docked mode, but we show no notifications.
  UpdateDisplay("400x400");
  EXPECT_TRUE(GetDisplayNotificationText().empty());
  EXPECT_TRUE(GetDisplayNotificationAdditionalText().empty());
}

// Tests that rotation notifications are only shown when the rotation source is
// a user action. The accelerometer source nevber produces any notifications.
TEST_F(ScreenLayoutObserverTest, RotationNotification) {
  Shell::Get()->screen_layout_observer()->set_show_notifications_for_testing(
      true);
  UpdateDisplay("400x400");
  const int64_t primary_id =
      display_manager()->GetPrimaryDisplayCandidate().id();

  // The accelerometer source.
  display_manager()->SetDisplayRotation(
      primary_id, display::Display::ROTATE_90,
      display::Display::ROTATION_SOURCE_ACCELEROMETER);
  EXPECT_TRUE(GetDisplayNotificationText().empty());
  EXPECT_TRUE(GetDisplayNotificationAdditionalText().empty());

  // The user source.
  display_manager()->SetDisplayRotation(primary_id,
                                        display::Display::ROTATE_180,
                                        display::Display::ROTATION_SOURCE_USER);
  EXPECT_EQ(l10n_util::GetStringFUTF16(
                IDS_ASH_STATUS_TRAY_DISPLAY_ROTATED, GetFirstDisplayName(),
                l10n_util::GetStringUTF16(
                    IDS_ASH_STATUS_TRAY_DISPLAY_ORIENTATION_180)),
            GetDisplayNotificationAdditionalText());

  // The active source.
  display_manager()->SetDisplayRotation(
      primary_id, display::Display::ROTATE_270,
      display::Display::ROTATION_SOURCE_ACTIVE);
  EXPECT_EQ(l10n_util::GetStringFUTF16(
                IDS_ASH_STATUS_TRAY_DISPLAY_ROTATED, GetFirstDisplayName(),
                l10n_util::GetStringUTF16(
                    IDS_ASH_STATUS_TRAY_DISPLAY_ORIENTATION_270)),
            GetDisplayNotificationAdditionalText());

  // Switch to Tablet
  Shell::Get()->tablet_mode_controller()->EnableTabletModeWindowManager(true);

  // The accelerometer source.
  display_manager()->SetDisplayRotation(
      primary_id, display::Display::ROTATE_90,
      display::Display::ROTATION_SOURCE_ACCELEROMETER);
  EXPECT_TRUE(GetDisplayNotificationText().empty());

  // The user source.
  display_manager()->SetDisplayRotation(primary_id,
                                        display::Display::ROTATE_180,
                                        display::Display::ROTATION_SOURCE_USER);
  EXPECT_EQ(l10n_util::GetStringFUTF16(
                IDS_ASH_STATUS_TRAY_DISPLAY_ROTATED, GetFirstDisplayName(),
                l10n_util::GetStringUTF16(
                    IDS_ASH_STATUS_TRAY_DISPLAY_ORIENTATION_180)),
            GetDisplayNotificationAdditionalText());

  // The active source.
  display_manager()->SetDisplayRotation(
      primary_id, display::Display::ROTATE_270,
      display::Display::ROTATION_SOURCE_ACTIVE);
  EXPECT_TRUE(GetDisplayNotificationText().empty());
}

TEST_F(ScreenLayoutObserverTest, MirrorModeAddOrRemoveDisplayMessage) {
  Shell::Get()->screen_layout_observer()->set_show_notifications_for_testing(
      true);

  const int64_t internal_display_id =
      display::test::DisplayManagerTestApi(display_manager())
          .SetFirstDisplayAsInternalDisplay();
  constexpr int first_display_id = 11;
  constexpr int second_display_id = 12;
  display::ManagedDisplayInfo first_display_info =
      display::CreateDisplayInfo(first_display_id, gfx::Rect(1, 1, 500, 500));
  display::ManagedDisplayInfo second_display_info =
      display::CreateDisplayInfo(second_display_id, gfx::Rect(2, 2, 500, 500));
  std::vector<display::ManagedDisplayInfo> display_info_list;
  display_info_list.push_back(display::CreateDisplayInfo(
      internal_display_id, gfx::Rect(0, 0, 100, 100)));
  display_info_list.push_back(first_display_info);
  display_info_list.push_back(second_display_info);
  display_manager()->OnNativeDisplaysChanged(display_info_list);

  // Mirroring across 3 displays.
  display_manager()->SetMirrorMode(display::MirrorMode::kNormal, base::nullopt);
  EXPECT_EQ(l10n_util::GetStringFUTF16(IDS_ASH_STATUS_TRAY_DISPLAY_MIRRORING,
                                       GetMirroringDisplayNames()),
            GetDisplayNotificationText());

  // Mirror mode persists when a display is removed.
  CloseNotification();
  display_info_list.erase(display_info_list.end() - 1);
  display_manager()->OnNativeDisplaysChanged(display_info_list);
  EXPECT_TRUE(GetDisplayNotificationText().empty());
  EXPECT_TRUE(display_manager()->IsInMirrorMode());

  // Turn off mirror mode.
  CloseNotification();
  display_manager()->SetMirrorMode(display::MirrorMode::kOff, base::nullopt);
  EXPECT_EQ(l10n_util::GetStringUTF16(IDS_ASH_STATUS_TRAY_DISPLAY_MIRROR_EXIT),
            GetDisplayNotificationText());

  // Turn on mirror mode.
  CloseNotification();
  display_manager()->SetMirrorMode(display::MirrorMode::kNormal, base::nullopt);
  EXPECT_EQ(l10n_util::GetStringFUTF16(IDS_ASH_STATUS_TRAY_DISPLAY_MIRRORING,
                                       GetMirroringDisplayNames()),
            GetDisplayNotificationText());

  // Mirror mode ends when only one display is left.
  CloseNotification();
  display_info_list.erase(display_info_list.end() - 1);
  display_manager()->OnNativeDisplaysChanged(display_info_list);
  EXPECT_EQ(l10n_util::GetStringUTF16(IDS_ASH_STATUS_TRAY_DISPLAY_MIRROR_EXIT),
            GetDisplayNotificationText());

  // Mirror mode is restored when the display is reconncted.
  CloseNotification();
  display_info_list.push_back(first_display_info);
  display_manager()->OnNativeDisplaysChanged(display_info_list);
  EXPECT_EQ(l10n_util::GetStringFUTF16(IDS_ASH_STATUS_TRAY_DISPLAY_MIRRORING,
                                       GetMirroringDisplayNames()),
            GetDisplayNotificationText());

  // Add the other display, the mirror mode persists.
  CloseNotification();
  display_info_list.push_back(second_display_info);
  display_manager()->OnNativeDisplaysChanged(display_info_list);
  EXPECT_TRUE(GetDisplayNotificationText().empty());
  EXPECT_TRUE(display_manager()->IsInMirrorMode());
}

}  // namespace ash
