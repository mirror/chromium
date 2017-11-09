// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/user_manager/scoped_user_manager.h"

#include "components/user_manager/user_manager.h"

namespace user_manager {

ScopedUserManager::ScopedUserManager(UserManager* user_manager) {
  if (user_manager::UserManager::GetForTesting())
    user_manager::UserManager::GetForTesting()->Shutdown();

  previous_user_manager_ =
      user_manager::UserManager::SetForTesting(user_manager);
}

ScopedUserManager::~ScopedUserManager() {
  // Shutdown and destroy current UserManager instance that we track.
  user_manager::UserManager::Get()->Shutdown();
  delete user_manager::UserManager::Get();
  user_manager::UserManager::SetInstance(nullptr);

  user_manager::UserManager::SetForTesting(previous_user_manager_);
}

}  // namespace user_manager
