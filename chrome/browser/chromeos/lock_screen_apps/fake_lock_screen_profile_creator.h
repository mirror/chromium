// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_CHROMEOS_LOCK_SCREEN_APPS_FAKE_LOCK_SCREEN_PROFILE_CREATOR_H_
#define CHROME_BROWSER_CHROMEOS_LOCK_SCREEN_APPS_FAKE_LOCK_SCREEN_PROFILE_CREATOR_H_

#include "base/macros.h"
#include "chrome/browser/chromeos/lock_screen_apps/lock_screen_profile_creator.h"

class TestingProfileManager;

namespace lock_screen_apps {

class FakeLockScreenProfileCreator : public LockScreenProfileCreator {
 public:
  explicit FakeLockScreenProfileCreator(TestingProfileManager* profile_manager);
  ~FakeLockScreenProfileCreator() override;

  void CreateProfile();
  void SetProfileCreationFailed();

 protected:
  void InitializeImpl() override;

 private:
  TestingProfileManager* profile_manager_;

  DISALLOW_COPY_AND_ASSIGN(FakeLockScreenProfileCreator);
};

}  // namespace lock_screen_apps

#endif  // CHROME_BROWSER_CHROMEOS_LOCK_SCREEN_APPS_FAKE_LOCK_SCREEN_PROFILE_CREATOR_H_
