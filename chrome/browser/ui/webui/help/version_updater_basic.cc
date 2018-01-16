// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/webui/help/version_updater_basic.h"

#include "base/strings/string16.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/upgrade_detector.h"

void VersionUpdaterBasic::CheckForUpdate(
    const StatusCallback& status_callback,
    const PromoteCallback&) {
  Status status = DISABLED;
  if (g_browser_process->upgrade_detector() &&
      g_browser_process->upgrade_detector()->notify_upgrade()) {
    status = NEARLY_UPDATED;
  }
  status_callback.Run(status, 0, std::string(), 0, base::string16());
}

VersionUpdater* VersionUpdater::Create(content::WebContents* web_contents) {
  return new VersionUpdaterBasic;
}
