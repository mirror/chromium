// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_WEBUI_CHROMEOS_LOGIN_ENCRYPTION_MIGRATION_UTILS_H_
#define CHROME_BROWSER_UI_WEBUI_CHROMEOS_LOGIN_ENCRYPTION_MIGRATION_UTILS_H_

#include <string>
#include <vector>

namespace chromeos {

// Returns the blacklist of user profile subdirectories to be skipped during
// minimal migration.
void GetMinimalMigrationUserPathsBlacklist(
    std::vector<std::string>* user_paths_blacklist);

}  // namespace chromeos

#endif  // CHROME_BROWSER_UI_WEBUI_CHROMEOS_LOGIN_ENCRYPTION_MIGRATION_UTILS_H_
