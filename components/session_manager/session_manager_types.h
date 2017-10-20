// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_SESSION_MANAGER_SESSION_MANAGER_TYPES_H_
#define COMPONENTS_SESSION_MANAGER_SESSION_MANAGER_TYPES_H_

#include "components/signin/core/account_id/account_id.h"

namespace session_manager {

// TODO(xiyuan): Get rid/consolidate with chromeos::LoggedInState.
enum class SessionState {
  // Default value, when session state hasn't been initialized yet.
  UNKNOWN = 0,

  // Running out of box UI.
  OOBE,

  // Running login UI (primary user). User sign in might be completed when
  // either some session initialization is happening or user has to go through
  // some UI flow on the same login UI like select avatar, agree to terms of
  // service etc.
  LOGIN_PRIMARY,

  // A transient state before browser launch.
  LOGGED_IN_NOT_ACTIVE,

  // A user(s) has logged in *and* login UI is hidden i.e. user session is
  // not blocked.
  ACTIVE,

  // The session screen is locked.
  LOCKED,

  // Same as SESSION_STATE_LOGIN_PRIMARY but for multi-profiles sign in i.e.
  // when there's at least one user already active in the session.
  LOGIN_SECONDARY,
};

// A type for session id.
using SessionId = int;

// Info about a user session.
struct Session {
  SessionId id;
  AccountId user_account_id;
};

// Limits the number of logged in users to 5. User-switcher UI was not designed
// around a large number of users. This also helps on memory-constrained
// devices. See b/64593342 for some additional context.
constexpr int kMaximumNumberOfUserSessions = 5;

}  // namespace session_manager

#endif  // COMPONENTS_SESSION_MANAGER_SESSION_MANAGER_TYPES_H_
