// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_CHROMEOS_ARC_POLICY_ARC_POLICY_UTIL_H_
#define CHROME_BROWSER_CHROMEOS_ARC_POLICY_ARC_POLICY_UTIL_H_

#include <stdint.h>

class Profile;

namespace arc {
namespace policy_util {

// The action that should be taken when an ecryptfs user home which needs
// migration is detected. This must match the order/values of the
// EcryptfsMigrationStrategy policy.
enum class EcryptfsMigrationAction : int32_t {
  // Don't migrate.
  kDisallowMigration,
  // Migrate without asking the user.
  kMigrate,
  // Wipe the user home and start again.
  kWipe,
  // Ask the user if migration should be performed.
  kAskUser
};
constexpr int32_t kEcryptfsMigrationActionCount =
    static_cast<int32_t>(EcryptfsMigrationAction::kAskUser);

// Returns true if the account is managed. Otherwise false.
bool IsAccountManaged(Profile* profile);

// Returns true if ARC is disabled by --enterprise-diable-arc flag.
bool IsArcDisabledForEnterprise();

}  // namespace policy_util
}  // namespace arc

#endif  // CHROME_BROWSER_CHROMEOS_ARC_POLICY_ARC_POLICY_UTIL_H_
