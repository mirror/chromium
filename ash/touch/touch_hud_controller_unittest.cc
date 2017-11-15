// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/touch/touch_hud_controller.h"

#include "ash/public/cpp/ash_pref_names.h"
#include "ash/session/session_controller.h"
#include "ash/session/test_session_controller_client.h"
#include "ash/shell.h"
#include "ash/test/ash_test_base.h"
#include "components/prefs/pref_service.h"

namespace ash {
namespace {

constexpr char kUser1[] = "user1@test.com";
constexpr char kUser2[] = "user2@test.com";

class TouchHudControllerTest : public AshTestBase {
 public:
  TouchHudControllerTest() = default;
  ~TouchHudControllerTest() override = default;

  PrefService* GetPrefs(const std::string& email) {
    return Shell::Get()->session_controller()->GetUserPrefServiceForUser(
        AccountId::FromUserEmail(email));
  }

  void SwitchActiveUser(const std::string& email) {
    GetSessionControllerClient()->SwitchActiveUser(
        AccountId::FromUserEmail(email));
  }

  // AshTestBase:
  void SetUp() override {
    AshTestBase::SetUp();
    GetSessionControllerClient()->AddUserSession(kUser1);
    GetSessionControllerClient()->AddUserSession(kUser2);
    SwitchActiveUser(kUser1);
  }

 private:
  DISALLOW_COPY_AND_ASSIGN(TouchHudControllerTest);
};

TEST_F(TouchHudControllerTest, Basics) {
  TouchHudController* hud = Shell::Get()->touch_hud_controller();
  ASSERT_TRUE(hud);

  // Projection is off by default.
  EXPECT_FALSE(hud->IsTouchHudProjectionEnabled());

  // Projection can be toggled.
  hud->SetTouchHudProjectionEnabled(true);
  EXPECT_TRUE(hud->IsTouchHudProjectionEnabled());
  hud->SetTouchHudProjectionEnabled(false);
  EXPECT_FALSE(hud->IsTouchHudProjectionEnabled());
}

TEST_F(TouchHudControllerTest, MultiUser) {
  TouchHudController* hud = Shell::Get()->touch_hud_controller();
  // Projection is off by default.
  EXPECT_FALSE(hud->IsTouchHudProjectionEnabled());

  // Enable for the first user.
  hud->SetTouchHudProjectionEnabled(true);
  EXPECT_TRUE(hud->IsTouchHudProjectionEnabled());

  // Switch users. Projection is off for user 2.
  SwitchActiveUser(kUser2);
  EXPECT_FALSE(hud->IsTouchHudProjectionEnabled());

  // Switch back to user 1. Projection is back on.
  SwitchActiveUser(kUser1);
  EXPECT_TRUE(hud->IsTouchHudProjectionEnabled());

  // Preferences are set for each user.
  EXPECT_TRUE(GetPrefs(kUser1)->GetBoolean(prefs::kTouchHudProjectionEnabled));
  EXPECT_FALSE(GetPrefs(kUser2)->GetBoolean(prefs::kTouchHudProjectionEnabled));
}

}  // namespace
}  // namespace ash
