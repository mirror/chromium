// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/lifetime/application_lifetime_platform_default.h"

#include "base/callback.h"
#include "chrome/browser/lifetime/application_lifetime_platform.h"
#include "chrome/browser/ui/user_manager.h"
#include "chrome/common/pref_names.h"
#include "components/prefs/pref_service.h"

namespace chrome {
namespace platform {

void AttemptUserExit(base::OnceClosure attempt_user_exit_finish) {
  UserManager::Hide();
  std::move(attempt_user_exit_finish).Run()
}

void PreAttemptExit() {
  // If we know that all browsers can be closed without blocking,
  // don't notify users of crashes beyond this point.
  // Note that MarkAsCleanShutdown() does not set UMA's exit cleanly bit
  // so crashes during shutdown are still reported in UMA.
  // Android doesn't use Browser.
  if (AreAllBrowsersCloseable())
    MarkAsCleanShutdown();
}

void PreAttemptRelaunch() {}

void AttemptRestartFinish() {
  // Set the flag to restore state after the restart.
  pref_service->SetBoolean(prefs::kRestartLastSessionOnShutdown, true);
  AttemptExit();
}

}  // namespace platform
}  // namespace chrome
