// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_USER_MANAGER_SCOPED_USER_MANAGER_H_
#define COMPONENTS_USER_MANAGER_SCOPED_USER_MANAGER_H_

#include <memory>

#include "base/macros.h"

namespace user_manager {

class UserManager;

// Helper class for unit tests. Initializes the UserManager singleton to the
// given |user_manager| and tears it down again on destruction.
class ScopedUserManager {
 public:
  explicit ScopedUserManager(std::unique_ptr<UserManager> user_manager);
  ~ScopedUserManager();

 private:
  const std::unique_ptr<UserManager> user_manager_;

  DISALLOW_COPY_AND_ASSIGN(ScopedUserManager);
};

}  // namespace user_manager

#endif  // COMPONENTS_USER_MANAGER_SCOPED_USER_MANAGER_H_
