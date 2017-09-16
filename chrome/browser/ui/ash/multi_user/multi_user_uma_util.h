// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASH_MULTI_PROFILE_UMA_H_
#define ASH_MULTI_PROFILE_UMA_H_

#include "base/macros.h"

namespace multi_user_uma_util {
// Keep these enums up to date with tools/metrics/histograms/histograms.xml.
enum SessionMode {
  SESSION_SINGLE_USER_MODE = 0,
  SESSION_SIDE_BY_SIDE_MODE,
  SESSION_SEPARATE_DESKTOP_MODE,
  NUM_SESSION_MODES
};

enum SigninUserAction {
  SIGNIN_USER_BY_TRAY = 0,
  SIGNIN_USER_BY_BROWSER_FRAME,
  NUM_SIGNIN_USER_ACTIONS
};

enum TeleportWindowType {
  TELEPORT_WINDOW_BROWSER = 0,
  TELEPORT_WINDOW_INCOGNITO_BROWSER,
  TELEPORT_WINDOW_V1_APP,
  TELEPORT_WINDOW_V2_APP,
  TELEPORT_WINDOW_PANEL,
  TELEPORT_WINDOW_POPUP,
  TELEPORT_WINDOW_UNKNOWN,
  NUM_TELEPORT_WINDOW_TYPES
};

enum TeleportWindowAction {
  TELEPORT_WINDOW_DRAG_AND_DROP = 0,
  TELEPORT_WINDOW_CAPTION_MENU,
  TELEPORT_WINDOW_RETURN_BY_MINIMIZE,
  TELEPORT_WINDOW_RETURN_BY_LAUNCHER,
  NUM_TELEPORT_WINDOW_ACTIONS
};

// Record the type of user (multi profile) session.
void RecordSessionMode(SessionMode mode);

// Record signing in a new user and what UI path was taken.
void RecordSigninUser(SigninUserAction action);

// Record the type of window which got teleported to another desk.
void RecordTeleportWindowType(TeleportWindowType window_type);

// Record the way and how many times a window got teleported to another desk.
void RecordTeleportAction(TeleportWindowAction action);

// Record number of users joined into a session. Called every time a user gets
// added.
void RecordUserCount(int number_of_users);

// Record a discarded tab in the number of running users bucket.
void RecordDiscardedTab(int number_of_users);

}  // namespace multi_user_uma_util

#endif  // ASH_MULTI_PROFILE_UMA_H_
