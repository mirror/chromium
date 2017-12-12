// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/lifetime/application_lifetime_chromeos.h"

#include "base/barrier_closure.h"
#include "base/bind.h"
#include "base/callback.h"
#include "chrome/browser/chromeos/profiles/profile_helper.h"
#include "components/prefs/pref_service.h"
#include "components/user_manager/user_manager.h"

namespace chromeos {

void StartUserPrefsCommitPendingWrite(base::OnceClosure on_finish) {
  DCHECK(user_manager::UserManager::Get());
  const user_manager::UserList& users =
      user_manager::UserManager::Get()->GetLoggedInUsers();
  base::RepeatingClosure on_finish_barrier(
      base::BarrierClosure(users.size(), std::move(on_finish)));
  for (const user_manager::User* user : users) {
    Profile* profile = ProfileHelper::Get()->GetProfileByUser(user);
    if (profile) {
      profile->GetPrefs()->CommitPendingWrite(on_finish_barrier);
    } else {
      // There is no prefs writer to trigger barrier for this profile.
      on_finish_barrier.Run();
    }
  }
}

}  // namespace chromeos
