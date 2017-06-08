// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/command_line.h"
#include "google_apis/drive/drive_switches.h"

namespace google_apis {
// Enables or disables Team Drives integration.
const char kEnableTeamDrives[] = "team-drives";
// Use allTeamDrives corpus when fetching list of files.
const char kEnableTeamDrivesAllTeamDrivesCorpus[] =
    "team-drives-all-team-drives-corpus";

TeamDrivesIntegrationStatus GetTeamDrivesIntegrationSwitch() {
  return base::CommandLine::ForCurrentProcess()->HasSwitch(kEnableTeamDrives) ?
      TEAM_DRIVES_INTEGRATION_ENABLED : TEAM_DRIVES_INTEGRATION_DISABLED;
}

bool GetTeamDrivesAllTeamDrivesCorpus() {
  return base::CommandLine::ForCurrentProcess()->HasSwitch(
      kEnableTeamDrivesAllTeamDrivesCorpus);
}

}  // namespace google_apis
