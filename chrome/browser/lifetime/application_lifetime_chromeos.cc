// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/lifetime/application_lifetime_chromeos.h"

#include "base/bind.h"
#include "base/callback.h"
#include "chrome/browser/chromeos/profiles/profile_helper.h"
#include "components/prefs/pref_service.h"
#include "components/user_manager/user_manager.h"

namespace chromeos {

namespace {

void Next(base::OnceClosure on_finish, size_t next_user_index) {
  const user_manager::UserList& users =
      user_manager::UserManager::Get()->GetLoggedInUsers();
  if (next_user_index >= users.size()) {
    std::move(on_finish).Run();
    return;
  }
  Profile* profile =
      ProfileHelper::Get()->GetProfileByUser(users[next_user_index]);
  if (profile) {
    profile->GetPrefs()->CommitPendingWrite(
        base::BindOnce(&Next, std::move(on_finish), next_user_index + 1));
  } else {
    Next(std::move(on_finish), next_user_index + 1);
  }
}

}  // anonymous namespace

void StartUserPrefsCommitPendingWriteOnExit(base::OnceClosure on_finish) {
  Next(std::move(on_finish), 0);
}

}  // namespace chromeos
