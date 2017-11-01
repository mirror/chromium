// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/user_manager/scoped_user_manager.h"

#include <utility>

#include "base/logging.h"
#include "components/user_manager/user_manager.h"

namespace user_manager {

ScopedUserManager::ScopedUserManager(std::unique_ptr<UserManager> user_manager)
    : user_manager_(std::move(user_manager)) {
  DCHECK(!user_manager::UserManager::IsInitialized());
  user_manager_->Initialize();
}

ScopedUserManager::~ScopedUserManager() {
  DCHECK_EQ(user_manager::UserManager::Get(), user_manager_.get());
  user_manager_->Shutdown();
  user_manager_->Destroy();
}

}  // namespace user_manager
