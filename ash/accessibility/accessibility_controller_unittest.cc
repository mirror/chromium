// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/accessibility/accessibility_controller.h"

#include "ash/accessibility/test_accessibility_controller_client.h"
#include "ash/ash_constants.h"
#include "ash/public/cpp/ash_pref_names.h"
#include "ash/public/cpp/config.h"
#include "ash/session/session_controller.h"
#include "ash/shell.h"
#include "ash/system/accessibility_observer.h"
#include "ash/system/tray/system_tray_notifier.h"
#include "ash/test/ash_test_base.h"
#include "chromeos/dbus/dbus_thread_manager.h"
#include "chromeos/dbus/fake_power_manager_client.h"
#include "components/prefs/pref_service.h"

namespace ash {

void CopyResult(base::TimeDelta* dest, base::TimeDelta src) {
  *dest = src;
}

class TestAccessibilityObserver : public AccessibilityObserver {
 public:
  TestAccessibilityObserver() = default;
  ~TestAccessibilityObserver() override = default;

  // AccessibilityObserver:
  void OnAccessibilityStatusChanged(
      AccessibilityNotificationVisibility notify) override {
    if (notify == A11Y_NOTIFICATION_NONE) {
      ++notification_none_changed_;
    } else if (notify == A11Y_NOTIFICATION_SHOW) {
      ++notification_show_changed_;
    }
  }

  int notification_none_changed_ = 0;
  int notification_show_changed_ = 0;

 private:
  DISALLOW_COPY_AND_ASSIGN(TestAccessibilityObserver);
};

class AccessibilityControllerTest : public AshTestBase {
 public:
  AccessibilityControllerTest() = default;
  ~AccessibilityControllerTest() override = default;

  void SetUp() override {
    auto power_manager_client =
        std::make_unique<chromeos::FakePowerManagerClient>();
    power_manager_client_ = power_manager_client.get();
    chromeos::DBusThreadManager::GetSetterForTesting()->SetPowerManagerClient(
        std::move(power_manager_client));

    AshTestBase::SetUp();
  }

 protected:
  // Owned by chromeos::DBusThreadManager.
  chromeos::FakePowerManagerClient* power_manager_client_ = nullptr;

 private:
  DISALLOW_COPY_AND_ASSIGN(AccessibilityControllerTest);
};

TEST_F(AccessibilityControllerTest, PrefsAreRegistered) {
  PrefService* prefs =
      Shell::Get()->session_controller()->GetLastActiveUserPrefService();
  EXPECT_TRUE(prefs->FindPreference(prefs::kAccessibilityAutoclickEnabled));
  EXPECT_TRUE(prefs->FindPreference(prefs::kAccessibilityAutoclickDelayMs));
  EXPECT_TRUE(prefs->FindPreference(prefs::kAccessibilityHighContrastEnabled));
  EXPECT_TRUE(prefs->FindPreference(prefs::kAccessibilityLargeCursorEnabled));
  EXPECT_TRUE(prefs->FindPreference(prefs::kAccessibilityLargeCursorDipSize));
  EXPECT_TRUE(prefs->FindPreference(prefs::kAccessibilityMonoAudioEnabled));
  EXPECT_TRUE(
      prefs->FindPreference(prefs::kAccessibilityScreenMagnifierEnabled));
  EXPECT_TRUE(
      prefs->FindPreference(prefs::kAccessibilitySpokenFeedbackEnabled));
}

TEST_F(AccessibilityControllerTest, SetAutoclickEnabled) {
  AccessibilityController* controller =
      Shell::Get()->accessibility_controller();
  EXPECT_FALSE(controller->IsAutoclickEnabled());

  TestAccessibilityObserver observer;
  Shell::Get()->system_tray_notifier()->AddAccessibilityObserver(&observer);
  EXPECT_EQ(0, observer.notification_none_changed_);

  controller->SetAutoclickEnabled(true);
  EXPECT_TRUE(controller->IsAutoclickEnabled());
  EXPECT_EQ(1, observer.notification_none_changed_);

  controller->SetAutoclickEnabled(false);
  EXPECT_FALSE(controller->IsAutoclickEnabled());
  EXPECT_EQ(2, observer.notification_none_changed_);

  Shell::Get()->system_tray_notifier()->RemoveAccessibilityObserver(&observer);
}

TEST_F(AccessibilityControllerTest, SetHighContrastEnabled) {
  AccessibilityController* controller =
      Shell::Get()->accessibility_controller();
  EXPECT_FALSE(controller->IsHighContrastEnabled());

  TestAccessibilityObserver observer;
  Shell::Get()->system_tray_notifier()->AddAccessibilityObserver(&observer);
  EXPECT_EQ(0, observer.notification_none_changed_);

  controller->SetHighContrastEnabled(true);
  EXPECT_TRUE(controller->IsHighContrastEnabled());
  EXPECT_EQ(1, observer.notification_none_changed_);

  controller->SetHighContrastEnabled(false);
  EXPECT_FALSE(controller->IsHighContrastEnabled());
  EXPECT_EQ(2, observer.notification_none_changed_);

  Shell::Get()->system_tray_notifier()->RemoveAccessibilityObserver(&observer);
}

TEST_F(AccessibilityControllerTest, SetLargeCursorEnabled) {
  AccessibilityController* controller =
      Shell::Get()->accessibility_controller();
  EXPECT_FALSE(controller->IsLargeCursorEnabled());

  TestAccessibilityObserver observer;
  Shell::Get()->system_tray_notifier()->AddAccessibilityObserver(&observer);
  EXPECT_EQ(0, observer.notification_none_changed_);

  controller->SetLargeCursorEnabled(true);
  EXPECT_TRUE(controller->IsLargeCursorEnabled());
  EXPECT_EQ(1, observer.notification_none_changed_);

  controller->SetLargeCursorEnabled(false);
  EXPECT_FALSE(controller->IsLargeCursorEnabled());
  EXPECT_EQ(2, observer.notification_none_changed_);

  Shell::Get()->system_tray_notifier()->RemoveAccessibilityObserver(&observer);
}

TEST_F(AccessibilityControllerTest, DisableLargeCursorResetsSize) {
  PrefService* prefs =
      Shell::Get()->session_controller()->GetLastActiveUserPrefService();
  EXPECT_EQ(kDefaultLargeCursorSize,
            prefs->GetInteger(prefs::kAccessibilityLargeCursorDipSize));

  // Simulate using chrome settings webui to turn on large cursor and set a
  // custom size.
  prefs->SetBoolean(prefs::kAccessibilityLargeCursorEnabled, true);
  prefs->SetInteger(prefs::kAccessibilityLargeCursorDipSize, 48);

  // Turning off large cursor resets the size to the default.
  prefs->SetBoolean(prefs::kAccessibilityLargeCursorEnabled, false);
  EXPECT_EQ(kDefaultLargeCursorSize,
            prefs->GetInteger(prefs::kAccessibilityLargeCursorDipSize));
}

TEST_F(AccessibilityControllerTest, SetMonoAudioEnabled) {
  AccessibilityController* controller =
      Shell::Get()->accessibility_controller();
  EXPECT_FALSE(controller->IsMonoAudioEnabled());

  TestAccessibilityObserver observer;
  Shell::Get()->system_tray_notifier()->AddAccessibilityObserver(&observer);
  EXPECT_EQ(0, observer.notification_none_changed_);

  controller->SetMonoAudioEnabled(true);
  EXPECT_TRUE(controller->IsMonoAudioEnabled());
  EXPECT_EQ(1, observer.notification_none_changed_);

  controller->SetMonoAudioEnabled(false);
  EXPECT_FALSE(controller->IsMonoAudioEnabled());
  EXPECT_EQ(2, observer.notification_none_changed_);

  Shell::Get()->system_tray_notifier()->RemoveAccessibilityObserver(&observer);
}

TEST_F(AccessibilityControllerTest, SetSpokenFeedbackEnabled) {
  AccessibilityController* controller =
      Shell::Get()->accessibility_controller();
  EXPECT_FALSE(controller->IsSpokenFeedbackEnabled());

  TestAccessibilityObserver observer;
  Shell::Get()->system_tray_notifier()->AddAccessibilityObserver(&observer);
  EXPECT_EQ(0, observer.notification_none_changed_);
  EXPECT_EQ(0, observer.notification_show_changed_);

  controller->SetSpokenFeedbackEnabled(true, A11Y_NOTIFICATION_SHOW);
  EXPECT_TRUE(controller->IsSpokenFeedbackEnabled());
  EXPECT_EQ(0, observer.notification_none_changed_);
  EXPECT_EQ(1, observer.notification_show_changed_);

  controller->SetSpokenFeedbackEnabled(false, A11Y_NOTIFICATION_NONE);
  EXPECT_FALSE(controller->IsSpokenFeedbackEnabled());
  EXPECT_EQ(1, observer.notification_none_changed_);
  EXPECT_EQ(1, observer.notification_show_changed_);

  Shell::Get()->system_tray_notifier()->RemoveAccessibilityObserver(&observer);
}

// Tests that ash's controller gets shutdown sound duration properly from
// remote client.
TEST_F(AccessibilityControllerTest, GetShutdownSoundDuration) {
  AccessibilityController* controller =
      Shell::Get()->accessibility_controller();
  TestAccessibilityControllerClient client;
  controller->SetClient(client.CreateInterfacePtrAndBind());

  base::TimeDelta sound_duration;
  controller->PlayShutdownSound(
      base::BindOnce(&CopyResult, base::Unretained(&sound_duration)));
  controller->FlushMojoForTest();
  EXPECT_EQ(TestAccessibilityControllerClient::kShutdownSoundDuration,
            sound_duration);
}

TEST_F(AccessibilityControllerTest, SetDarkenScreen) {
  ASSERT_FALSE(power_manager_client_->backlights_forced_off());

  AccessibilityController* controller =
      Shell::Get()->accessibility_controller();
  controller->SetDarkenScreen(true);
  EXPECT_TRUE(power_manager_client_->backlights_forced_off());

  controller->SetDarkenScreen(false);
  EXPECT_FALSE(power_manager_client_->backlights_forced_off());
}

using AccessibilityControllerSigninTest = NoSessionAshTestBase;

TEST_F(AccessibilityControllerSigninTest, SigninScreenPrefs) {
  AccessibilityController* accessibility =
      Shell::Get()->accessibility_controller();

  SessionController* session = Shell::Get()->session_controller();
  EXPECT_EQ(session_manager::SessionState::LOGIN_PRIMARY,
            session->GetSessionState());
  EXPECT_FALSE(accessibility->IsLargeCursorEnabled());

  // Verify that toggling prefs at the signin screen changes the signin setting.
  PrefService* signin_prefs = session->GetSigninScreenPrefService();
  using prefs::kAccessibilityLargeCursorEnabled;
  EXPECT_FALSE(signin_prefs->GetBoolean(kAccessibilityLargeCursorEnabled));
  accessibility->SetLargeCursorEnabled(true);
  EXPECT_TRUE(accessibility->IsLargeCursorEnabled());
  EXPECT_TRUE(signin_prefs->GetBoolean(kAccessibilityLargeCursorEnabled));

  // Verify that toggling prefs after signin changes the user setting.
  SimulateUserLogin("user1@test.com");
  PrefService* user_prefs = session->GetLastActiveUserPrefService();
  EXPECT_NE(signin_prefs, user_prefs);
  EXPECT_FALSE(accessibility->IsLargeCursorEnabled());
  EXPECT_FALSE(user_prefs->GetBoolean(kAccessibilityLargeCursorEnabled));
  accessibility->SetLargeCursorEnabled(true);
  EXPECT_TRUE(accessibility->IsLargeCursorEnabled());
  EXPECT_TRUE(user_prefs->GetBoolean(kAccessibilityLargeCursorEnabled));
}

}  // namespace ash
