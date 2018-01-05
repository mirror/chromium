// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/public/cpp/wallpaper_utils.h"

#include "chromeos/login/login_state.h"

namespace ash {
namespace wallpaper_utils {

bool ShouldShowWallpaperSetting() {
  const chromeos::LoginState* login_state = chromeos::LoginState::Get();
  if (!login_state->IsUserLoggedIn())
    return false;

  const chromeos::LoginState::LoggedInUserType user_type =
      login_state->GetLoggedInUserType();
  // Whitelist user types that are allowed to change their wallpaper. (Guest
  // users are not, see crosbug.com/26900.)
  if (user_type == chromeos::LoginState::LOGGED_IN_USER_REGULAR ||
      user_type == chromeos::LoginState::LOGGED_IN_USER_OWNER ||
      user_type == chromeos::LoginState::LOGGED_IN_USER_PUBLIC_ACCOUNT ||
      user_type == chromeos::LoginState::LOGGED_IN_USER_SUPERVISED) {
    return true;
  }
  return false;
}

}  // namespace wallpaper_utils
}  // namespace ash