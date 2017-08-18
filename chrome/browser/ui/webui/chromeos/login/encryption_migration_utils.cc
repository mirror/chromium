// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/webui/chromeos/login/encryption_migration_utils.h"

#include "chrome/common/chrome_constants.h"
#include "chromeos/chromeos_constants.h"
#include "content/public/common/content_constants.h"
#include "extensions/common/constants.h"

namespace chromeos {

// Blacklist of user profile subdirectories to be skipped during minimal
// migration.
void GetMinimalMigrationUserPathsBlacklist(
    std::vector<std::string>* user_paths_blacklist) {
  user_paths_blacklist->push_back(extensions::kLocalAppSettingsDirectoryName);
  user_paths_blacklist->push_back(
      extensions::kLocalExtensionSettingsDirectoryName);
  user_paths_blacklist->push_back(extensions::kSyncAppSettingsDirectoryName);
  user_paths_blacklist->push_back(
      extensions::kSyncExtensionSettingsDirectoryName);
  user_paths_blacklist->push_back(extensions::kManagedSettingsDirectoryName);
  user_paths_blacklist->push_back(extensions::kStateStoreName);
  user_paths_blacklist->push_back(extensions::kRulesStoreName);
  user_paths_blacklist->push_back(extensions::kInstallDirectoryName);
  user_paths_blacklist->push_back(chrome::kCacheDirname);
  user_paths_blacklist->push_back(content::kAppCacheDirname);
  user_paths_blacklist->push_back(content::kPepperDataDirname);
  user_paths_blacklist->push_back(chromeos::kDriveCacheDirname);
  user_paths_blacklist->push_back(chrome::kGCMStoreDirname);

  // Constants for the following paths are currently not accessible (e.g. in
  // namespaces)
  user_paths_blacklist->push_back("log");
  user_paths_blacklist->push_back("crash");
  user_paths_blacklist->push_back("GPUCache");
  user_paths_blacklist->push_back("Local Storage");
  user_paths_blacklist->push_back("Session Storage");
  user_paths_blacklist->push_back("Thumbnails");
  user_paths_blacklist->push_back("arc.apps");
  user_paths_blacklist->push_back("Downloads");
  user_paths_blacklist->push_back("DownloadMetadata");
  user_paths_blacklist->push_back("Service Worker");
}

}  // namespace chromeos
