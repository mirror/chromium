// Copyright (c) 2006-2008 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_NOTIFICATIONS_USER_ACTIVITY_H_
#define CHROME_BROWSER_NOTIFICATIONS_USER_ACTIVITY_H_

#ifdef ENABLE_BACKGROUND_TASK

// Categorization of the user mode
enum UserMode {
  // User is away
  // * A screen saver is displayed.
  // * The machine is locked.
  // * Non-active Fast User Switching session is on (Windows XP & Vista).
  USER_AWAY_MODE = 0,

  // User is idle
  USER_IDLE_MODE,

  // User is interrupted by some critical event
  // * Laptop power is being suspended.
  USER_INTERRUPTED_MODE,

  // User is in not idle
  USER_NORMAL_MODE,

  // User is in full-screen presentation mode
  // * A full-screen application is running.
  // * A full-screen (exclusive mode) Direct3D application is running
  //  (Windows XP & Vista).
  // * The user activates Windows presentation settings (Windows Vista).
  USER_PRESENTATION_MODE,

  // User mode is undetermined
  USER_MODE_UNKNOWN,
};

class UserActivityObserver {
 public:
  // Called when there is any change to user activity.
  virtual void OnUserActivityChange() = 0;
};

class UserActivityInterface {
 public:
  // Add the observer to be notified when a user mode has been changed.
  virtual void AddObserver(UserActivityObserver* observer) = 0;

  // Check the user activity immediately.
  virtual void CheckNow() = 0;

  // Get the user activity value. Note that this is the latest cached value.
  virtual UserMode user_mode() const = 0;

  // Returns the number of milliseconds the system has had no user input.
  virtual uint32 QueryUserIdleTimeMs() = 0;
};

// Is user active?
inline bool IsActiveUserMode(UserMode user_mode) {
  return user_mode == USER_NORMAL_MODE;
}

#endif  // ENABLE_BACKGROUND_TASK
#endif  // CHROME_BROWSER_NOTIFICATIONS_USER_ACTIVITY_H_
