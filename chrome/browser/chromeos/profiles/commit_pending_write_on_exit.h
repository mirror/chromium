// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_CHROMEOS_PROFILES_COMMIT_PENDING_WRITE_ON_EXIT_H_
#define CHROME_BROWSER_CHROMEOS_PROFILES_COMMIT_PENDING_WRITE_ON_EXIT_H_

#include <stddef.h>

#include "base/callback_forward.h"
#include "base/macros.h"

namespace chromeos {

// This helper class is used to flush all profile data before exiting.
class UsersCommitPendingWriteOnExit {
 public:
  UsersCommitPendingWriteOnExit();
  ~UsersCommitPendingWriteOnExit();

  // Initiates flush. on_finish will be called when done.
  void Start(base::OnceClosure on_finish);

 private:
  // Schedules slush user with given index.
  void Next(base::OnceClosure on_finish, size_t next_user_index);

  DISALLOW_COPY_AND_ASSIGN(UsersCommitPendingWriteOnExit);
};

}  // namespace chromeos

#endif  // CHROME_BROWSER_CHROMEOS_PROFILES_COMMIT_PENDING_WRITE_ON_EXIT_H_
