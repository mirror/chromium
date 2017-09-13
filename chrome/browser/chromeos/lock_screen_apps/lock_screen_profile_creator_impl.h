// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_CHROMEOS_LOCK_SCREEN_APPS_LOCK_SCREEN_PROFILE_CREATOR_IMPL_H_
#define CHROME_BROWSER_CHROMEOS_LOCK_SCREEN_APPS_LOCK_SCREEN_PROFILE_CREATOR_IMPL_H_

#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "base/scoped_observer.h"
#include "chrome/browser/chromeos/lock_screen_apps/lock_screen_profile_creator.h"
#include "chrome/browser/chromeos/note_taking_helper.h"
#include "chrome/browser/profiles/profile.h"

namespace base {
class TickClock;
class TimeTicks;
}  // namespace base

namespace lock_screen_apps {

class LockScreenProfileCreatorImpl
    : public LockScreenProfileCreator,
      public chromeos::NoteTakingHelper::Observer {
 public:
  LockScreenProfileCreatorImpl(Profile* primary_profile,
                               base::TickClock* tick_clock);
  ~LockScreenProfileCreatorImpl() override;

  // chromeos::NoteTakingHelper::Observer:
  void OnAvailableNoteTakingAppsUpdated() override;
  void OnPreferredNoteTakingAppUpdated(Profile* profile) override;

 protected:
  void InitializeImpl() override;

 private:
  void OnExtensionSystemReady();
  void OnProfileReady(const base::TimeTicks& start_time,
                      Profile* profile,
                      Profile::CreateStatus status);

  Profile* primary_profile_;
  base::TickClock* tick_clock_;

  ScopedObserver<chromeos::NoteTakingHelper,
                 chromeos::NoteTakingHelper::Observer>
      note_taking_helper_observer_;

  base::WeakPtrFactory<LockScreenProfileCreatorImpl> weak_ptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(LockScreenProfileCreatorImpl);
};

}  // namespace lock_screen_apps

#endif  // CHROME_BROWSER_CHROMEOS_LOCK_SCREEN_APPS_LOCK_SCREEN_PROFILE_CREATOR_IMPL_H_
