// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_LIFETIME_APPLICATION_LIFETIME_CHROMEOS_H_
#define CHROME_BROWSER_LIFETIME_APPLICATION_LIFETIME_CHROMEOS_H_

#include "base/callback_forward.h"

namespace chromeos {

void StartUserPrefsCommitPendingWriteOnExit(base::OnceClosure on_finish);

}  // namespace chromeos

#endif  // CHROME_BROWSER_LIFETIME_APPLICATION_LIFETIME_CHROMEOS_H_
