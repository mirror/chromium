// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_CHROMEOS_PROFILES_COMMIT_PENDING_WRITE_ON_EXIT_H_
#define CHROME_BROWSER_CHROMEOS_PROFILES_COMMIT_PENDING_WRITE_ON_EXIT_H_

#include <stddef.h>

#include "base/callback_forward.h"
#include "base/macros.h"

namespace chromeos {
namespace flush_on_exit {

// Initiates flush all profile data of all logged in users.
void StartUsersCommitPendingWriteOnExit(base::OnceClosure on_finish);

}  // namespace flush_on_exit
}  // namespace chromeos

#endif  // CHROME_BROWSER_CHROMEOS_PROFILES_COMMIT_PENDING_WRITE_ON_EXIT_H_
