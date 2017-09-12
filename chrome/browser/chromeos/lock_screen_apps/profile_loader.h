// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_CHROMEOS_LOCK_SCREEN_APPS_PROFILE_LOADER_H_
#define CHROME_BROWSER_CHROMEOS_LOCK_SCREEN_APPS_PROFILE_LOADER_H_

#include <list>

#include "base/callback_forward.h"
#include "base/macros.h"

class Profile;

namespace lock_screen_apps {

class ProfileLoader {
 public:
  ProfileLoader();
  virtual ~ProfileLoader();

  void Initialize();

  void AddLoadProfileCallback(base::OnceClosure callback);

  bool Initialized() const;
  bool LoadingProfile() const;
  bool ProfileLoaded() const;

  Profile* lock_screen_profile() const { return lock_screen_profile_; }

 protected:
  virtual void InitializeImpl() = 0;

  void OnLockScreenProfileLoadStarted();
  void OnLockScreenProfileLoaded(Profile* lock_screen_profile);

 private:
  enum class State { kInitial, kInitialized, kLoadingProfile, kProfileLoaded };

  State state_ = State::kInitial;

  Profile* lock_screen_profile_;

  std::list<base::OnceClosure> load_profile_callbacks_;

  DISALLOW_COPY_AND_ASSIGN(ProfileLoader);
};

}  // namespace lock_screen_apps

#endif  // CHROME_BROWSER_CHROMEOS_LOCK_SCREEN_APPS_PROFILE_LOADER_H_
