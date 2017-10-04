// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_USER_MANAGER_SCOPED_USER_MANAGER_ENABLER_H_
#define COMPONENTS_USER_MANAGER_SCOPED_USER_MANAGER_ENABLER_H_

#include "base/macros.h"
#include "components/user_manager/user_manager_export.h"

namespace user_manager {

class UserManager;

// Helper class for unit tests. Initializes the UserManager singleton to the
// given |user_manager| and tears it down again on destruction. If the singleton
// had already been initialized, its previous value is restored after tearing
// down |user_manager|.
class USER_MANAGER_EXPORT ScopedUserManagerEnabler {
 public:
  // Takes ownership of |user_manager|.
  explicit ScopedUserManagerEnabler(UserManager* user_manager);
  ~ScopedUserManagerEnabler();

 private:
  user_manager::UserManager* previous_user_manager_;

  DISALLOW_COPY_AND_ASSIGN(ScopedUserManagerEnabler);
};

}  // namespace user_manager

#endif  // COMPONENTS_USER_MANAGER_SCOPED_USER_MANAGER_ENABLER_H_
