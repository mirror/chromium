// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_CHROMEOS_LOCK_SCREEN_APPS_LOCK_SCREEN_PROFILE_CREATOR_H_
#define CHROME_BROWSER_CHROMEOS_LOCK_SCREEN_APPS_LOCK_SCREEN_PROFILE_CREATOR_H_

#include <list>

#include "base/callback_forward.h"
#include "base/macros.h"

class Profile;

namespace lock_screen_apps {

class LockScreenProfileCreator {
 public:
  LockScreenProfileCreator();
  virtual ~LockScreenProfileCreator();

  void Initialize();

  void AddCreateProfileCallback(base::OnceClosure callback);

  bool Initialized() const;
  bool ProfileCreated() const;

  Profile* lock_screen_profile() const { return lock_screen_profile_; }

 protected:
  virtual void InitializeImpl() = 0;

  void OnLockScreenProfileCreateStarted();
  void OnLockScreenProfileCreated(Profile* lock_screen_profile);

 private:
  enum class State {
    kInitial,
    kInitialized,
    kCreatingProfile,
    kProfileCreated
  };

  State state_ = State::kInitial;

  Profile* lock_screen_profile_ = nullptr;

  std::list<base::OnceClosure> create_profile_callbacks_;

  DISALLOW_COPY_AND_ASSIGN(LockScreenProfileCreator);
};

}  // namespace lock_screen_apps

#endif  // CHROME_BROWSER_CHROMEOS_LOCK_SCREEN_APPS_LOCK_SCREEN_PROFILE_CREATOR_H_
